<?php

class RLPAClient
{
    private static $have_shell_mode = false;

    private $name;
    private $shutdown;
    private $socket;
    private $packet;

    private $process;
    private $process_stdin;
    private $process_stdout;
    private $process_stderr;

    private $workmode;

    public function __construct($socket)
    {
        $this->shutdown = false;
        $this->socket = $socket;
        $this->packet = new RLPAPacket();

        $this->process = null;
        $this->process_stdin = null;
        $this->process_stdout = null;
        $this->process_stderr = null;

        $this->workmode = self::WORKMODE_NONE;

        stream_set_blocking($this->socket, false);
        $this->name = stream_socket_get_name($this->socket, true);
        $motd = (new RLPAPacket(RLPAPacket::TAG_MESSAGEBOX, "Welcome, {$this->name}"))->pack();
        fwrite($this->socket, $motd);
    }

    private function processPacket()
    {
        switch ($this->packet->tag) {
            case RLPAPacket::TAG_APDU:
                if ($this->process_stdin) {
                    fwrite($this->process_stdin, json_encode(['type' => 'apdu', 'payload' => ['ecode' => 0, 'data' => bin2hex($this->packet->value)]]) . PHP_EOL);
                }
                break;
            case RLPAPacket::TAG_MANAGEMNT:
                if (self::$have_shell_mode) {
                    $hint = (new RLPAPacket(RLPAPacket::TAG_MESSAGEBOX, "Already have one shell mode client"))->pack();
                    fwrite($this->socket, $hint);
                    throw new Exception();
                }
                self::$have_shell_mode = true;
                $this->workmode = self::WORKMODE_SHELL;
                stream_set_blocking(STDIN, false);
                while (fread(STDIN, 128));
                echo "lpac> ";
                break;
            default:
                $hint = (new RLPAPacket(RLPAPacket::TAG_MESSAGEBOX, "Unimplemented command."))->pack();
                fwrite($this->socket, $hint);
                throw new Exception();
                break;
        }
    }

    public function onSocketRecv()
    {
        if ($this->shutdown) {
            return;
        }

        try {
            $this->packet->recv($this->socket);
        } catch (\Exception $e) {
            $this->close();
            throw $e;
        }

        if (!$this->packet->isFinished()) {
            return;
        }

        $this->processPacket();

        $this->packet = new RLPAPacket();
    }

    public function onProcessStdout()
    {
        $stdout = fgets($this->process_stdout);
        if (strlen($stdout) == 0) {
            throw new Exception();
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
                        $packet = (new RLPAPacket(RLPAPacket::TAG_APDU, hex2bin($request['payload']['param'])))->pack();
                        fwrite($this->socket, $packet);
                        break;
                }
                break;
            case 'lpa':
                print_r($request);
                $this->processClose();
                while (fread(STDIN, 128));
                echo "lpac> ";
                break;
            default:
                print_r($request);
                break;
        }
    }

    public function onProcessStderr()
    {
    }

    public function onStdin()
    {
        if ($this->process !== null) {
            while (fread(STDIN, 128));
            return;
        }
        $cmd = fgets(STDIN);
        $cmd = trim($cmd);
        if ($cmd == "exit") {
            throw new Exception();
        }
        $this->processOpenLpac($cmd);
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

        if ($this->workmode == self::WORKMODE_SHELL) {
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

        if ($this->socket) {
            $disconnect = (new RLPAPacket(RLPAPacket::TAG_CLOSE))->pack();
            fwrite($this->socket, $disconnect);
            stream_socket_shutdown($this->socket,  STREAM_SHUT_RDWR);
            $this->socket = null;
        }

        $this->processClose();

        switch ($this->workmode) {
            case self::WORKMODE_SHELL:
                self::$have_shell_mode = false;
                break;
        }
    }

    public function getName()
    {
        return $this->name;
    }

    private function processOpenLpac($cmd)
    {
        $this->process = proc_open("./lpac {$cmd}", [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes, ".", ["APDU_INTERFACE" => "./libapduinterface_stdio.so"]);
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

    const WORKMODE_NONE = 0;
    const WORKMODE_SHELL = 1;
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

    const TAG_CLOSE = 0xFE;
    const TAG_APDU = 0xFF;
}

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
            echo "Accepted {$client->getName()}" . PHP_EOL;
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
            echo "Disconnected {$name}" . PHP_EOL;
        }
    }
}
