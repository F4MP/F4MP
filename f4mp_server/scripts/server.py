from ctypes import *

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

class librg_message(Structure):
    _fields_ = [
        ("id", c_uint32),
        ("ctx", c_void_p),
        ("data", c_void_p),

        ("peer", c_void_p),
        ("packet", c_void_p),

        ("user_data", c_void_p),
    ]

dll = cdll.LoadLibrary('bin/f4mp.dll')

dll.server_create.restype = c_void_p
dll.server_create.argtypes = c_char_p, c_uint32

@CFUNCTYPE(None, c_void_p)
def on_connection_request(event):
    print('asdf')
@CFUNCTYPE(None, c_void_p)
def on_connect_accepted(event):
    pass
@CFUNCTYPE(None, c_void_p)
def on_connect_refused(event):
    pass
@CFUNCTYPE(None, c_void_p)
def on_connect_disconnect(event):
    pass

@CFUNCTYPE(None, c_void_p)
def on_entity_create(event):
    pass
@CFUNCTYPE(None, c_void_p)
def on_entity_remove(event):
    pass
@CFUNCTYPE(None, c_void_p)
def on_entity_update(event):
    pass
@CFUNCTYPE(None, c_void_p)
def on_client_update(event):
    pass

@CFUNCTYPE(None, c_void_p)
def on_fire_weapon(msg):
    print(dll.data_ru32(c_void_p(cast(msg, POINTER(librg_message)).contents.data)))

if __name__ == '__main__':
    server = c_void_p(dll.server_create(b'localhost', 7779))

    dll.event_add(server, LIBRG_CONNECTION_REQUEST, on_connection_request)
    dll.event_add(server, LIBRG_CONNECTION_ACCEPT, on_connect_accepted)
    dll.event_add(server, LIBRG_CONNECTION_REFUSE, on_connect_refused)
    dll.event_add(server, LIBRG_CONNECTION_DISCONNECT, on_connect_disconnect)

    dll.event_add(server, LIBRG_ENTITY_CREATE, on_entity_create)
    dll.event_add(server, LIBRG_ENTITY_REMOVE, on_entity_remove)
    dll.event_add(server, LIBRG_ENTITY_UPDATE, on_entity_update)
    dll.event_add(server, LIBRG_CLIENT_STREAMER_UPDATE, on_client_update)

    dll.network_add(server, FireWeapon, on_fire_weapon)

    dll.server_start(server)
    
    while True:
        dll.server_tick(server)
    dll.server_destroy(server)