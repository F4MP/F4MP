import ctypes


class Peer(ctypes.Structure):
    _fields_ = [
        ("unused", ctypes.c_byte * (16 + 8 + 16 + 16 + 32 + 8 + 8)),
        ("address", ctypes.c_byte * 20),
    ]


class Data(ctypes.Structure):
    _fields_ = [
        ("capacity", ctypes.c_uint64),
        ("read_pos", ctypes.c_uint64),
        ("write_pos", ctypes.c_uint64),
        ("rawptr", ctypes.c_void_p),
        ("allocator", ctypes.c_byte * 16)
    ]


class Entity(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint32),
        ("type", ctypes.c_uint32),
        ("flags", ctypes.c_uint64),
        ("position", ctypes.c_float * 3),
        ("stream_range", ctypes.c_float),
        ("user_data", ctypes.c_void_p),
        ("stream_branch", ctypes.c_void_p),
        ("visibility", ctypes.c_byte * 16),
        ("virtual_world", ctypes.c_uint32),
        ("last_snapshot", ctypes.c_byte * 16),
        ("client_peer", ctypes.POINTER(Peer)),
        ("control_peer", ctypes.POINTER(Peer)),
        ("control_generation", ctypes.c_uint8),
        ("last_query", ctypes.POINTER(ctypes.c_uint32))
    ]


class Address(ctypes.Structure):
    _fields_ = [
        ("port", ctypes.c_int32),
        ("host", ctypes.c_char_p)
    ]


class _StreamsStruct(ctypes.Structure):
    _fields_ = [
        ("stream_input", Data),
        ("stream_output", Data),
        ("stream_upd_reliable", Data),
        ("stream_upd_unreliable", Data)
    ]


class Context(ctypes.Structure):
    class Network(ctypes.Structure):
        _fields_ = [
            ("peer", ctypes.POINTER(Peer)),
            ("host", ctypes.c_void_p),
            ("connected_peers", ctypes.c_byte * 16),
            ("last_address", Address),
            ("created", ctypes.c_int32),
            ("connected", ctypes.c_int32)
        ]

    class Entity(ctypes.Structure):
        _fields_ = [
            ("count", ctypes.c_uint32),
            ("cursor", ctypes.c_uint32),
            ("visibility", ctypes.c_byte * 16),
            ("list", ctypes.POINTER(Entity)),
            ("remove_queue", ctypes.POINTER(ctypes.c_uint32)),
            ("add_control_queue", ctypes.POINTER(ctypes.c_void_p))
        ]

    class StreamsUnion(ctypes.Union):
        _anonymous_ = ("struct",)
        _fields_ = [
            ("struct", _StreamsStruct),
            ("streams", Data * 4)
        ]

    _anonymous_ = ("streams",)
    _fields_ = [
        ("mode", ctypes.c_uint16),
        ("tick_delay", ctypes.c_double),
        ("max_connections", ctypes.c_uint16),
        ("max_entities", ctypes.c_uint32),
        ("world_size", ctypes.c_float * 3),
        ("min_branch_size", ctypes.c_float * 3),
        ("last_update", ctypes.c_double),
        ("user_data", ctypes.c_void_p),
        ("network", Network),
        ("entity", Entity),
        ("streams", StreamsUnion),
        ("timesync", ctypes.c_byte * 80),
        ("buffer_size", ctypes.c_uint64),
        ("buffer_timer", ctypes.c_void_p),
        ("buffer", ctypes.c_byte * 48),
        ("messages", ctypes.c_void_p),
        ("allocator", ctypes.c_byte * 16),
        ("timers", ctypes.c_byte * 8),
        ("events", ctypes.c_byte * 16),
        ("world", ctypes.c_byte * 96)
    ]


class Event(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint32),
        ("ctx", ctypes.POINTER(Context)),
        ("data", ctypes.POINTER(Data)),
        ("entity", ctypes.POINTER(Entity)),

        ("peer", ctypes.POINTER(Peer)),

        ("flags", ctypes.c_uint64),
        ("user_data", ctypes.c_void_p),
    ]


class Message(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint32),
        ("ctx", ctypes.POINTER(Context)),
        ("data", ctypes.POINTER(Data)),

        ("peer", ctypes.c_void_p),
        ("packet", ctypes.c_void_p),

        ("user_data", ctypes.c_void_p),
    ]
