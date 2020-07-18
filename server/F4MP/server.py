import ctypes
from F4MP import Librg, Handler


class Server:
    def __init__(self, address: str, port: int):
        self.address = address
        self.port = port
        self.ctx = ctypes.POINTER(Librg.Context)(Librg.Context())
        Librg.init(self.ctx)
        self.task_queue = []
        self.call_map = {}
        self.callback_hander = Handler.CallbackHandler(self)

    def listener(self, name=None):
        def decorator(func):
            self.call_map[name or func.__name__].add(func)
            return func

        return decorator

    def is_client(self):
        return Librg.is_client(self.ctx)

    def is_connected(self):
        return Librg.is_connected(self.ctx)

    def start(self):
        Librg.network_start(self.ctx, self.address.encode(), self.port)

    def tick(self):
        Librg.tick(self.ctx)

    def run(self):
        self.start()
        while True:
            for task in self.task_queue:
                for func in self.call_map[task.type]:
                    func(task.event)
                self.task_queue.remove(task)
            self.tick()
