from F4MP import Server

my_server = Server("127.0.0.1", 7779)


@my_server.listener()
def on_connection_request(event):
    print("Connection")


my_server.start()

