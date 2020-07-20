import F4MP

client = F4MP.Server("127.0.0.1", 7779)
client.ctx.contents.mode = 1
client.start()

while True:
    client.tick()
    print(client.is_connected())
