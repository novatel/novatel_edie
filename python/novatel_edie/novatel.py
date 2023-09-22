import json
from ctypes import *

from . import _util

DECODERS_DLL = _util.load_shared_library("decoders_dynamic_library")

# Patch c_bool be 4 bytes in size
temp_c_bool = c_bool
c_bool = c_uint32

DLL_MSG_FRAME_SIZE = DECODERS_DLL.get_frame_size()

DECODERS_DLL.version.restype = c_char_p
DECODERS_DLL.version.argtypes = None
DECODERS_DLL.pretty_version.restype = c_char_p
DECODERS_DLL.pretty_version.argtypes = None

DECODERS_DLL.framer_init.restype = c_void_p
DECODERS_DLL.framer_init.argtypes = [c_void_p, c_void_p]
DECODERS_DLL.framer_del.restype = None
DECODERS_DLL.framer_del.argtypes = [c_void_p]
DECODERS_DLL.framer_read.restype = None
DECODERS_DLL.framer_read.argtypes = [c_void_p, c_void_p, c_void_p]

DECODERS_DLL.decoder_init.restype = c_void_p
DECODERS_DLL.decoder_init.argtypes = [c_void_p, c_void_p, c_void_p]
DECODERS_DLL.decoder_del.restype = None
DECODERS_DLL.decoder_del.argtypes = [c_void_p]
DECODERS_DLL.decoder_read.restype = c_bool
DECODERS_DLL.decoder_read.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p, c_void_p]
DECODERS_DLL.decoder_copy_header.restype = c_void_p
DECODERS_DLL.decoder_copy_header.argtypes = [c_void_p, c_void_p]
DECODERS_DLL.decoder_copy_body.restype = c_void_p
DECODERS_DLL.decoder_copy_body.argtypes = [c_void_p, c_void_p]


class StreamReadStatus(Structure):
    """StreamReadStatus is the equivalent to the StreamReadStatus in the dll.

    The __init__ with allocate a pointer to a StreamReadStatus object. The __del__ will delete the memory of this
    object. Currently fields in this object are read only.
    """
    _fields_ = [("stream_percentage_read", c_uint),
                ("stream_current_read", c_uint),
                ("stream_length", c_ulonglong),
                ("stream_end", c_bool)]

    def __str__(self):
        return '{} {} {} {}'.format(self.stream_percentage_read,
                                    self.stream_current_read,
                                    self.stream_length,
                                    self.stream_end)


class Log:
    def __init__(self, header: dict = None, body: dict = None, unknown_data: bytes = None):
        self._header = header
        self._body = body
        self._unknown_data = unknown_data

    def __str__(self):
        return '#' + ','.join([str(x) for x in self.header.values()]) + \
               ';' + ','.join([str(x) for x in self.body.values()])

    def __getitem__(self, key):
        if self._header.get(key):
            return self._header.get(key)
        else:
            return None

    @property
    def header(self):
        return self._header

    @header.setter
    def header(self, header):
        self._header = header
        pass

    @property
    def body(self):
        return self._body

    @body.setter
    def body(self, body):
        self._body = body

    @property
    def unknown_data(self):
        return self._unknown_data

    @unknown_data.setter
    def unknown_data(self, unknown_data):
        self._unknown_data = unknown_data


class DecoderMessageHeader(Structure):
    """Message header that is returned when Decoding messages

    """
    FRAME_DATA_SIZE = DLL_MSG_FRAME_SIZE

    _fields_ = [
        ("name", c_char * 100),
        ("id", c_uint),
        ("port_id", c_ulong),
        ("length", c_uint),
        ("idle", c_double),
        ("week", c_ulong),
        ("time", c_ulong),
        ("message_format", c_ulong),
        ("time_status", c_ulong),
        ("receiver_status", c_ulong),
        ("sw_version", c_ulong),
        ("is_response", c_ulong)
    ]

    def __init__(self):
        values = dict()
        values['name'] = b''
        values['id'] = 0
        values['port_id'] = 0
        values['length'] = 0
        values['idle'] = 0
        values['week'] = 0
        values['time'] = 0
        values['message_format'] = 0
        values['time_status'] = 0
        values['receiver_status'] = 0
        values['sw_version'] = 0
        values['is_response'] = 0
        super(DecoderMessageHeader, self).__init__(**values)

    def __str__(self):
        return '{},{},{},{},{},{},{},{},{},{},{},{}'.format(string_at(self.name).decode(),
                                                            self.id,
                                                            self.port_id,
                                                            self.length,
                                                            self.idle,
                                                            self.week,
                                                            self.time,
                                                            self.message_format,
                                                            self.time_status,
                                                            self.receiver_status,
                                                            self.sw_version,
                                                            self.is_response)


class FramerMessageHeader(Structure):
    """Message header that is returned when Framing messages.

    FramerMessageHeader is identical to :class:`DecoderMessageHeader` with the addition of having the raw framed log
    data appended to the end of the header.

    """
    _fields_ = [("name", c_char * 100),
                ("id", c_uint),
                ("port_id", c_ulong),
                ("length", c_uint),
                ("idle", c_double),
                ("week", c_ulong),
                ("time", c_ulong),
                ("message_format", c_ulong),
                ("time_status", c_ulong),
                ("receiver_status", c_ulong),
                ("sw_version", c_ulong),
                ("is_response", c_ulong),
                ("log_data", (c_ubyte * DecoderMessageHeader.FRAME_DATA_SIZE))
                ]

    def __str__(self):
        return '{},{},{},{},{},{},{},{},{},{},{},{}'.format(string_at(self.name).decode(),
                                                            self.id,
                                                            self.port_id,
                                                            self.length,
                                                            self.idle,
                                                            self.week,
                                                            self.time,
                                                            self.message_format,
                                                            self.time_status,
                                                            self.receiver_status,
                                                            self.sw_version,
                                                            string_at(self.log_data))

    def get_data_str(self):
        return string_at(self.log_data)


class Framer:
    """Returns framed, but not decoded, messages.

    Data is returned in a :class:`FramerMessageHeader` object where most the log header information can be viewed and
    the raw log data.

    Args:
        input_stream (:obj): InputStream object exposed from the hw_interface module
    """

    def __init__(self, input_stream, filters=None):
        self._input_stream = input_stream.data_stream

        self._framer = DECODERS_DLL.framer_init(self._input_stream, filters)

        self._current_log = FramerMessageHeader()
        self._current_status = StreamReadStatus()

    def __del__(self):
        DECODERS_DLL.framer_del(self._framer)

    def __iter__(self):
        return self

    def next(self):
        """Get the next message from the framer

        Returns:
            (:obj:`FramerMessageHeader`): Framed data.
        """
        DECODERS_DLL.framer_read(self._framer, byref(self._current_status), byref(self._current_log))
        if self._current_status.stream_end:
            raise StopIteration
        elif self._current_status.stream_current_read == 0:
            return None
        else:
            return self._current_log

    __next__ = next


class Decoder:
    """Decodes NovAtel logs and return the decoded header and data to the user.

    Args:
        input_stream (:obj:`InputFileStream`): Stream to decode the data from
        json_database (str): Path to an alternative message database
        msg_filter (:obj:`Filter`, optional): Filters the returned decoded data.

    """

    def __init__(self, input_stream, json_database=_util.JSON_DB_PATH, msg_filter=None):
        self._Decoder = DECODERS_DLL.decoder_init(json_database.encode(),
                                                  input_stream.data_stream,
                                                  msg_filter)
        self._status: StreamReadStatus = StreamReadStatus()
        self._current_header = DecoderMessageHeader()
        self._msg_data = create_string_buffer(DLL_MSG_FRAME_SIZE)
        self._msg_json_data = create_string_buffer(DLL_MSG_FRAME_SIZE*2)
        self._ascii_current_body = str

        self._current_log = Log()
        self._current_log.header = self._current_header
        self._unknown_binary_id = c_uint32(-1).value

        self._timeout = 0

    def __del__(self):
        if self._Decoder:
            DECODERS_DLL.decoder_del(self._Decoder)
            self._Decoder = None

    def __iter__(self):
        return self

    @property
    def timeout(self):
        return self._timeout

    @timeout.setter
    def timeout(self, value):
        self._timeout = value

    def next(self) -> Log:
        """Returns the next log from EDIE

        Returns:
            :class:`Log`: Log object containing a decoded header and decoded body. If there is no decoder for the body
            then the body will just contain a pointer to the raw bytes of the framed log.

        """
        return self.read_decoded_message()

    __next__ = next

    def read_decoded_message(self, unknown_data=True):
        """Returns the next message from the data stream. If no message is available a StopItereation Exception is
        raised

        IMPORTANT: The returned log is volatile. DO NOT STORE the returned log. If you need a permanent copy of a log
        then call :func:`get_permanent_log`.
        Note that calling :func:`get_permanent_log` does consume memory. Calling it roughly 300k times will consume
        enough memory to use up all the addressable memory (~2Gb) on a 32-bit system which will crash the Python
        process. 64-bit environments will be able to consume all available system memory.

        CORRECT

        .. code-block:: python
            :emphasize-lines: 3

            for log in decoder:
                if log.header.name == 'VERSION':
                version_log = decoder.get_permanent_log()

        INCORRECT

        .. code-block:: python
            :emphasize-lines: 3

            for log in decoder:
                if log.header.name == 'VERSION':
                    version_log = log



        By placing a bit of memory management responsibility on the user the decoder system can be greatly optimized
        for speed.

        Returns:
            :class:`Log`: Log object containing a decoded header and decoded body. If there is no decoder for the body
            then the body will just contain a pointer to the raw bytes of the framed log.

        Raises:
            StopIteration: When EOF is reached

        """
        while True:
            read_status = DECODERS_DLL.decoder_read(self._Decoder,
                                                    byref(self._status),
                                                    byref(self._current_header),
                                                    self._msg_data,
                                                    self._msg_json_data)

            if self._status.stream_end:
                raise StopIteration
            try:
                edie_log = json.loads(self._msg_json_data.value)
            except Exception as ex:
                print(self._msg_json_data.value)
                continue
            if self._current_header.name == b'' or self._current_header.id == self._unknown_binary_id:
                if not unknown_data:
                    continue
                else:
                    return Log(edie_log['header'], unknown_data=bytes(self._msg_data.value))
            else:
                return Log(edie_log['header'], edie_log['body'])


__all__ = [
    "StreamReadStatus",
    "Log",
    "DecoderMessageHeader",
    "FramerMessageHeader",
    "Framer",
    "Decoder",
]
