class Item:
    def __init__(self, item_id, name, item_type):
        self.id = item_id
        self.name = name
        self.type = item_type


class Inventory:
    def __init__(self, items=None, equipped=None):
        """Represents the inventory of a Character. However is not tied to any specific Character.
        Args:
            items (List[Item]): List of items a Character has in its inventory
            equipped (List[Item]): List of items Character has equipped. Must be a subset of items.
        """
        assert set(equipped).issubset(set(items))
        self.items = items
        self.equipped = equipped


class SPECIAL:
    def __init__(self, strength, perception, endurance, charisma, intelligence, agility, luck):
        self.strength = strength
        self.perception = perception
        self.endurance = endurance
        self.charisma = charisma
        self.intelligence = intelligence
        self.agility = agility
        self.luck = luck


class Character:
    def __init__(self, id, position, hp, ap, special, inventory):
        """

        Args:
            id (int): Server Generated ID for the player instance
            position(Tuple[float, float, float]): 3D Vector
            hp(float):
            ap(float):
            special(SPECIAL):
            inventory(Inventory):
        """
        self.id = id
        self.position = position
        self.hp = hp
        self.ap = ap
        self.special = special
        self.inventory = inventory


class User:
    def __init__(self, id, username, connection, player: Character = None):
        self.id = id
        self.username = username
        self.connection = connection
        self.player = player


class Connection:
    def __init__(self, address, user: User = None):
        self.address = address
        self.user = user


class Event:
    def __init__(self, type, connection):
        self.type = type
        self.connection = connection
        self.user = connection.user
