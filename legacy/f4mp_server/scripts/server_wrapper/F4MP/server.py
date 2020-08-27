import asyncio
from F4MP import f4mp

Interface = f4mp.Interface


class Server:
    def __init__(self, address: str, port: int):
        self.instance = Interface.server_create(address, port)
        self.address = address
        self.port = port
        self.loop = asyncio.get_event_loop()
        self.task_queue = []
        self.call_map = {}
        self.callbacks = f4mp.Callbacks(self)

    def __del__(self):
        Interface.server_destroy(self.instance)

    def listener(self, name=None):
        def decorator(func):
            self.call_map[name or func.__name__].add(func)
            return func

        return decorator

    def start(self, tps=20):
        Interface.server_start(self.instance)
        self.loop_tasks(tps)

    def loop_tasks(self, tps):
        import time
        period = 1 / tps
        while True:
            start = time.time()
            for task in self.task_queue:
                for func in self.call_map[task.type]:
                    func(task.event)
            Interface.server_tick(self.instance)
            time.sleep(period - max(time.time() - start, 0))
