import os
import ctypes
from ctypes import CFUNCTYPE, c_char_p, c_uint32, c_void_p, c_uint64

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
dll = ctypes.cdll.LoadLibrary(os.path.join(BASE_DIR, "bin/f4mp.dll"))


class Message(ctypes.Structure):
    _fields_ = [
        ("id", c_uint32),
        ("ctx", c_void_p),
        ("data", c_void_p),

        ("peer", c_void_p),
        ("packet", c_void_p),

        ("user_data", c_void_p),
    ]


class Event(ctypes.Structure):
    _fields_ = [
        ("id", c_uint32),
        ("ctx", c_void_p),
        ("data", c_void_p),
        ("entity", c_void_p),

        ("peer", c_void_p),

        ("flags", c_uint64),
        ("user_data", c_void_p),
    ]


class Task:
    def __init__(self, e_type, event):
        self.type = e_type
        self.event = Event(event)


class CallbackEnum:
    LIBRG_CONNECTION_INIT = 0
    LIBRG_CONNECTION_REQUEST = 1
    LIBRG_CONNECTION_REFUSE = 2
    LIBRG_CONNECTION_ACCEPT = 3
    LIBRG_CONNECTION_DISCONNECT = 4
    LIBRG_CONNECTION_TIMEOUT = 5
    LIBRG_CONNECTION_TIMESYNC = 6

    LIBRG_ENTITY_CREATE = 7
    LIBRG_ENTITY_UPDATE = 8
    LIBRG_ENTITY_REMOVE = 9
    LIBRG_CLIENT_STREAMER_ADD = 10
    LIBRG_CLIENT_STREAMER_REMOVE = 11
    LIBRG_CLIENT_STREAMER_UPDATE = 12

    LIBRG_EVENT_LAST = 13

    Hit = LIBRG_EVENT_LAST + 1
    FireWeapon = LIBRG_EVENT_LAST + 2
    SpawnEntity = LIBRG_EVENT_LAST + 3
    SyncEntity = LIBRG_EVENT_LAST + 4
    SpawnBuilding = LIBRG_EVENT_LAST + 5
    RemoveBuilding = LIBRG_EVENT_LAST + 6
    Speak = LIBRG_EVENT_LAST + 7

    dict = {
        "on_connection_request": LIBRG_CONNECTION_REQUEST,
        "on_connection_accepted": LIBRG_CONNECTION_ACCEPT,
        "on_connection_refused": LIBRG_CONNECTION_REFUSE,
        "on_disconnect": LIBRG_CONNECTION_DISCONNECT,
        "on_entity_create": LIBRG_ENTITY_CREATE,
        "on_entity_delete": LIBRG_ENTITY_REMOVE,
        "on_entity_update": LIBRG_ENTITY_UPDATE,
        "on_client_update": LIBRG_CLIENT_STREAMER_UPDATE
    }

    def __getitem__(self, item):
        return self.dict[item]


class Callbacks:
    def __init__(self, server):
        self.server = server
        self.instance = server.instance
        self.callback_enum = CallbackEnum()

        for func_name, enum in self.callback_enum.dict.items():
            func = CFUNCTYPE(None, c_void_p)(getattr(self, func_name))
            self.server.call_map[func_name] = set()
            Interface.register_event(self.instance, enum, func)

    def on_connection_request(self, event):
        self.server.task_queue.append(Task("on_connection_request", event))

    def on_connection_accepted(self, event):
        self.server.task_queue.append(Task("on_connection_accepted", event))

    def on_connection_refused(self, event):
        self.server.task_queue.append(Task("on_connection_refused", event))

    def on_disconnect(self, event):
        self.server.task_queue.append(Task("on_disconnect", event))

    def on_entity_create(self, event):
        self.server.task_queue.append(Task("on_entity_create", event))

    def on_entity_delete(self, event):
        self.server.task_queue.append(Task("on_entity_delete", event))

    def on_entity_update(self, event):
        self.server.task_queue.append(Task("on_entity_update", event))

    def on_client_update(self, event):
        self.server.task_queue.append(Task("on_client_update", event))


class Interface:
    """F4MP DLL Interface"""
    dll.server_create.restype = c_void_p
    dll.server_create.argtypes = c_char_p, c_uint32

    @staticmethod
    def server_create(address: str, port: int) -> c_void_p:
        """Creates a server

        Arguments:
            address(str): The bind address for the server
            port(int): The port the server will listen on
        Note:
            The port
        """
        return c_void_p(dll.server_create(address.encode(), port))

    @staticmethod
    def server_start(instance: c_void_p) -> None:
        dll.server_start(instance)

    @staticmethod
    def server_destroy(instance: c_void_p) -> None:
        """Destroys the server"""
        dll.server_destroy(instance)

    @staticmethod
    def server_tick(instance: c_void_p) -> None:
        dll.server_tick(instance)

    @staticmethod
    def register_event(server: c_void_p, enum: int, func):
        dll.event_add(server, enum, func)

    @staticmethod
    def register_net(server: c_void_p, enum: int, func):
        dll.network_add(server, enum, func)
