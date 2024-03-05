<?php

class ShellMode extends RLPAWorkMode
{
    private function lpacPrompt()
    {
        while (fread(STDIN, 128));
        echo "lpac> ";
    }

    public function start($data)
    {
        $this->lpacPrompt();
    }

    public function onStdin($data)
    {
        $cmd = trim($data);
        if ($cmd == "exit") {
            throw new Exception();
        }
        $this->client->processOpenLpac($cmd);
    }

    public function onProcessFinished($data)
    {
        if ($data === null) {
            throw new Exception();
        }

        print_r($data);
        $this->lpacPrompt();
    }

    public function finished()
    {
        return false;
    }

    static public function requireStdin()
    {
        return true;
    }
}

class ProcessNotificationMode extends RLPAWorkMode
{
    private $state = 0;
    private $notifications = [];
    private $failed_count = 0;

    public function start($data)
    {
        $this->state = 0;
        $this->client->processOpenLpac("notification list");
    }

    private function processOneNotification()
    {
        $notification = array_shift($this->notifications);
        if ($notification == null) {
            if ($this->failed_count) {
                $this->client->messageBox("{$this->failed_count} notifications failed to process, please retry.");
            } else {
                $this->client->messageBox("All notifications processed successfully.");
            }
            $this->state = 2;
            return;
        }

        echo "Processing: seqNumber: {$notification['seqNumber']}, profileManagementOperation: {$notification['profileManagementOperation']}" . PHP_EOL;

        switch ($notification['profileManagementOperation']) {
            case 'install':
            case 'enable':
            case 'disable':
                $this->client->processOpenLpac("notification process {$notification['seqNumber']} 1");
                break;
            case 'delete':
                $this->client->processOpenLpac("notification process {$notification['seqNumber']} 0");
                break;
            default:
                throw new Exception();
        }
    }

    public function onProcessFinished($data)
    {
        if ($data === null) {
            throw new Exception();
        }

        switch ($this->state) {
            case 0:
                if ($data['code'] !== 0) {
                    throw new Exception();
                }
                $this->notifications = $data['data'] ?? [];
                $this->state = 1;
                $this->processOneNotification();
                break;
            case 1:
                if ($data['code'] == 0) {
                    echo "Process success" . PHP_EOL;
                } else {
                    echo "Process failed" . PHP_EOL;
                    $this->failed_count++;
                }
                $this->processOneNotification();
                break;
        }
    }

    public function finished()
    {
        return ($this->state == 2);
    }

    static public function requireStdin()
    {
        return false;
    }
}

class DownloadProfileMode extends RLPAWorkMode
{
    private $state = 0;

    public function start($data)
    {
        $this->state = 0;
        $data = ltrim($data, "LPA:");
        $data = explode(chr(0x02), $data); // GSM-8bit will convert $ to \x02

        print_r($data);

        if ($data[0] != "1") {
            $this->client->messageBox("Invalid format: version mismatch");
            throw new Exception();
        }

        if (!isset($data[1])) {
            $this->client->messageBox("Invalid format: missing SM-DP+");
            throw new Exception();
        }

        $cmd = 'profile download -s ' . escapeshellarg($data[1]);
        if (isset($data[2])) {
            $cmd .= ' -m ' . escapeshellarg($data[2]);
        }

        $this->client->processOpenLpac($cmd);
    }

    public function onProcessFinished($data)
    {
        if ($data === null) {
            throw new Exception();
        }

        if ($data['code'] == 0) {
            echo 'Download success' . PHP_EOL;
            $this->client->messageBox('Download success');
        } else {
            print_r($data);
            echo 'Download failed' . PHP_EOL;
            $this->client->messageBox('Download failed');
            $this->client->messageBox($data['message']);
            $this->client->messageBox($data['data']);
        }

        $this->state = 1;
    }

    public function finished()
    {
        return ($this->state == 1);
    }

    static public function requireStdin()
    {
        return false;
    }
}

class RLPAClient
{
    private static $stdin_lock = false;

    private $name;
    private $shutdown;
    private $socket;
    private $packet;

    private $process;
    private $process_stdin;
    private $process_stdout;
    private $process_stderr;

    /** @var RLPAWorkMode */
    private $workmode;

    public function __construct($socket)
    {
        $this->name = stream_socket_get_name($socket, true);

        $this->shutdown = false;
        $this->socket = $socket;
        $this->packet = new RLPAPacket();

        $this->process = null;
        $this->process_stdin = null;
        $this->process_stdout = null;
        $this->process_stderr = null;

        $this->workmode = null;

        stream_set_blocking($socket, false);
        $this->messageBox("Welcome, {$this->name}");
    }

    private function sendRLPAPacket($tag, $value = "")
    {
        if ($this->socket) {
            fwrite($this->socket, (new RLPAPacket($tag, $value))->pack());
        }
    }

    public function messageBox($msg)
    {
        $this->sendRLPAPacket(RLPAPacket::TAG_MESSAGEBOX, $msg);
    }

    public function lockAPDU()
    {
        $this->sendRLPAPacket(RLPAPacket::TAG_APDU_LOCK);
    }

    public function unlockAPDU()
    {
        $this->sendRLPAPacket(RLPAPacket::TAG_APDU_UNLOCK);
    }

    private function processPacket()
    {
        $work_class = null;

        if ($this->packet->tag === RLPAPacket::TAG_APDU) {
            if (!$this->process_stdin) {
                return;
            }
            fwrite($this->process_stdin, json_encode(['type' => 'apdu', 'payload' => ['ecode' => 0, 'data' => bin2hex($this->packet->value)]]) . PHP_EOL);
            return;
        }

        if ($this->workmode) {
            return;
        }

        switch ($this->packet->tag) {
            case RLPAPacket::TAG_MANAGEMNT:
                $work_class = ShellMode::class;
                break;
            case RLPAPacket::TAG_PROCESS_NOTIFICATION:
                $work_class = ProcessNotificationMode::class;
                break;
            case RLPAPacket::TAG_DOWNLOAD_PROFILE:
                $work_class = DownloadProfileMode::class;
                break;
            default:
                $this->messageBox("Unimplemented command.");
                throw new Exception();
                break;
        }

        if ($work_class === null) {
            throw new Exception();
        }

        if ($work_class::requireStdin()) {
            if (self::$stdin_lock) {
                $this->messageBox("Already have one shell mode client");
                throw new Exception();
            }
            stream_set_blocking(STDIN, false);
        }

        $this->workmode = new $work_class($this);
        $this->workmode->start($this->packet->value);
    }

    public function onSocketRecv()
    {
        if ($this->workmode && $this->workmode->finished()) {
            throw new Exception();
        }

        if ($this->shutdown) {
            return;
        }

        $this->packet->recv($this->socket);

        if (!$this->packet->isFinished()) {
            return;
        }

        $this->processPacket();

        $this->packet = new RLPAPacket();
    }

    public function onProcessStdout()
    {
        if ($this->workmode && $this->workmode->finished()) {
            throw new Exception();
        }

        $stdout = fgets($this->process_stdout);
        if (strlen($stdout) == 0) {
            $this->processClose();
            $this->workmode->onProcessFinished(null);
            if ($this->workmode->finished()) {
                throw new Exception();
            }
            return;
        }

        $request = json_decode($stdout, true);
        if (!$request) {
            return;
        }

        switch ($request['type']) {
            case 'apdu':
                switch ($request['payload']['func']) {
                    case 'connect':
                        fwrite($this->process_stdin, json_encode(['type' => 'apdu', 'payload' => ['ecode' => 0]]) . PHP_EOL);
                        break;
                    case 'logic_channel_open':
                        fwrite($this->process_stdin, json_encode(['type' => 'apdu', 'payload' => ['ecode' => 0]]) . PHP_EOL);
                        break;
                    case 'transmit':
                        fwrite($this->socket, (new RLPAPacket(RLPAPacket::TAG_APDU, hex2bin($request['payload']['param'])))->pack());
                        break;
                }
                break;
            case 'lpa':
                $this->processClose();
                $this->workmode->onProcessFinished($request['payload']);
                if ($this->workmode->finished()) {
                    throw new Exception();
                }
                break;
            default:
                // print_r($request);
                break;
        }
    }

    public function onProcessStderr()
    {
        if (!$this->process_stderr) {
            return;
        }

        echo fgets($this->process_stderr);
    }

    public function onStdin()
    {
        if ($this->process !== null) {
            while (fread(STDIN, 128));
            return;
        }
        if ($this->workmode::requireStdin()) {
            $this->workmode->onStdin(fgets(STDIN));
            if ($this->workmode->finished()) {
                throw new Exception();
            }
        }
    }

    public function getSelectable()
    {
        $selectable = [];

        if ($this->shutdown) {
            return null;
        }

        if ($this->socket) {
            $selectable[] = [$this->socket, [$this, 'onSocketRecv']];
        }

        if ($this->process_stdout) {
            $selectable[] = [$this->process_stdout, [$this, 'onProcessStdout']];
        }

        if ($this->process_stderr) {
            $selectable[] = [$this->process_stderr, [$this, 'onProcessStderr']];
        }

        if ($this->workmode && $this->workmode::requireStdin()) {
            $selectable[] = [STDIN, [$this, 'onStdin']];
        }

        return $selectable;
    }

    public function close()
    {
        if ($this->shutdown) {
            return;
        }

        $this->shutdown = true;

        $this->unlockAPDU();

        if ($this->socket) {
            $disconnect = (new RLPAPacket(RLPAPacket::TAG_CLOSE))->pack();
            fwrite($this->socket, $disconnect);
            stream_socket_shutdown($this->socket,  STREAM_SHUT_RDWR);
            $this->socket = null;
        }

        $this->processClose();

        if ($this->workmode && $this->workmode::requireStdin()) {
            self::$stdin_lock = false;
        }
    }

    public function getName()
    {
        return $this->name;
    }

    public function processOpenLpac($cmd)
    {
        $this->lockAPDU();
        $this->process = proc_open("./lpac {$cmd}", [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes, ".", ['APDU_INTERFACE' => './libapduinterface_stdio.so']);
        $this->process_stdin = $pipes[0];
        $this->process_stdout = $pipes[1];
        $this->process_stderr = $pipes[2];
        stream_set_blocking($this->process_stdout, false);
        stream_set_blocking($this->process_stderr, false);
    }

    private function processClose()
    {
        if ($this->process_stdin) {
            fclose($this->process_stdin);
            $this->process_stdin = null;
        }
        if ($this->process_stdout) {
            fclose($this->process_stdout);
            $this->process_stdout = null;
        }
        if ($this->process_stderr) {
            fclose($this->process_stderr);
            $this->process_stderr = null;
        }
        if ($this->process) {
            proc_close($this->process);
            $this->process = null;
        }
        $this->unlockAPDU();
    }

    public static function resource2id($resource)
    {
        $id = null;
        if (is_resource($resource)) {
            $id = 'R' . intval($resource);
        } else {
            $id = 'O' . spl_object_id($resource);
        }

        return $id;
    }

    public static function getSelectableTable($clients, &$read, &$callbackTable)
    {
        $read = [];
        $callbackTable = [];
        foreach ($clients as $c) {
            $rss = $c->getSelectable();
            foreach ($rss as $r) {
                $read[self::resource2id($r[0])] = $r[0];
                $callbackTable[self::resource2id($r[0])] = $r[1];
            }
        }
    }
}

class RLPAPacket
{
    public $tag;
    public $value;

    private $_state;
    private $_next_read_len;
    private $_buffer;

    public function __construct($tag = 0x00, $value = "")
    {
        $this->tag = $tag;
        $this->value = $value;
        $this->_state = 0;
        $this->_next_read_len = 1;
        $this->_buffer = "";
    }

    public function recv($socket)
    {
        if ($this->isFinished()) {
            return;
        }

        $buf = stream_socket_recvfrom($socket, $this->_next_read_len);
        if (strlen($buf) === 0) {
            throw new \Exception();
        }

        $this->_next_read_len -= strlen($buf);
        $this->_buffer .= $buf;

        if ($this->_next_read_len === 0) {
            switch ($this->_state) {
                case 0:
                    $this->tag = unpack('C', $this->_buffer)[1];
                    $this->_state = 1;
                    $this->_next_read_len = 2;
                    $this->_buffer = "";
                    break;
                case 1:
                    $this->_state = 2;
                    $this->_next_read_len = unpack('v', $this->_buffer)[1];
                    if ($this->_next_read_len >= (512 - 3)) {
                        throw new \Exception();
                    }
                    $this->_buffer = "";
                    if ($this->_next_read_len === 0) {
                        $this->_state = 3;
                        $this->value = "";
                    }
                    break;
                case 2:
                    $this->_state = 3;
                    $this->value = $this->_buffer;
                    $this->_buffer = "";
                    break;
            }
        }
    }

    public function isFinished()
    {
        return ($this->_state === 3);
    }

    public function pack()
    {
        $packet = "";
        $packet .= pack('C', $this->tag);
        $packet .= pack('v', strlen($this->value));
        $packet .= $this->value;

        return $packet;
    }

    const TAG_MESSAGEBOX = 0x00;
    const TAG_MANAGEMNT = 0x01;
    const TAG_DOWNLOAD_PROFILE = 0x02;
    const TAG_PROCESS_NOTIFICATION = 0x03;

    const TAG_REBOOT = 0xFB;
    const TAG_CLOSE = 0xFC;
    const TAG_APDU_LOCK = 0xFD;
    const TAG_APDU = 0xFE;
    const TAG_APDU_UNLOCK = 0xFF;
}

abstract class RLPAWorkMode
{
    protected $client;

    public function __construct(RLPAClient $client)
    {
        $this->client = $client;
    }

    public function start($data)
    {
    }

    public function onStdin($data)
    {
    }

    public function onProcessFinished($data)
    {
    }

    public function finished()
    {
        return true;
    }

    static public function requireStdin()
    {
        return false;
    }
};

gc_enable();
$g_clients = [];
$g_server_socket = stream_socket_server("tcp://0.0.0.0:1888");

while (1) {
    $read = [];
    $callbackTable = [];
    RLPAClient::getSelectableTable($g_clients, $read, $callbackTable);

    $read['server'] = $g_server_socket;
    $write = null;
    $exception = null;

    $ret = stream_select($read, $write, $exception, null);

    foreach ($read as $key => $socket) {
        if ($key === 'server') {
            $accepted = stream_socket_accept($socket);
            if (!$accepted) {
                continue;
            }
            $client = new RLPAClient($accepted);
            $g_clients[$client->getName()] = $client;
            echo PHP_EOL . "Accepted {$client->getName()}" . PHP_EOL;
            continue;
        }
        try {
            ($callbackTable[$key])();
        } catch (\Exception $e) {
            $instance = $callbackTable[$key][0];
            $instance->close();
            $name = $instance->getName();
            unset($instance);
            unset($g_clients[$name]);
            echo PHP_EOL . "Disconnected {$name}" . PHP_EOL;
        }
    }
}
