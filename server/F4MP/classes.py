class Item:
    def __init__(self, item_id, name, item_type):
        self.id = item_id
        self.name = name
        self.type = item_type


class Attributes:
    def __init__(self, strength, perception, endurance, charisma, intelligence, agility, luck, perks=None):
        self.strength = strength
        self.perception = perception
        self.endurance = endurance
        self.charisma = charisma
        self.intelligence = intelligence
        self.agility = agility
        self.luck = luck
        self.perks = perks


class Status:
    def __init__(self, hp: int, ap: int, max_hp, max_ap):
        assert hp <= max_hp and ap <= max_ap  # Probably not neccesary, could make HP a decimal between 0 and 1
        self.hp = hp
        self.ap = ap
        self.max_hp = max_hp
        self.max_ap = max_ap


class Connection:
    def __init__(self, address):
        self.address = address
        ...  # More here probably


class User:
    def __init__(self, user_id, username, position, connection, status, attributes, inventory):
        self.id = user_id
        self.username = username
        self.position = position
        self.connection = connection

        self.status = status
        self.attributes = attributes
        self.inventory = inventory
