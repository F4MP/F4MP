import ctypes
import F4MP
from F4MP import Librg, Handler

class Server:
    """F4MP Server Object"""

    def __init__(self, address: str, port: int):
        self.address = address
        self.port = port
        self.ctx = ctypes.POINTER(Librg.Context)(Librg.Context())
        self.connected_users = []
        Librg.init(self.ctx)
        self.call_map = {}
        self.callback_handler = Handler.CallbackHandler(self)

    def listener(self, name=None):
        def decorator(func):
            self.call_map[name or func.__name__].add(func)
            return func

        return decorator

    def handle(self, func, enum, event):
        ip_b = event.peer.contents.host.contents.address_host
        print(ip_b)
        connection = F4MP.Connection(
            address=str(ip_b)
        )
        event = F4MP.Event(enum, connection)
        func(event)

    def start(self):
        Librg.network_start(self.ctx, self.address.encode(), self.port)

    def tick(self):
        Librg.tick(self.ctx)

    def run(self):
        self.start()
        while True:
            self.tick()
