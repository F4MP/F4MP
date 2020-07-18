import os
import ctypes

from F4MP.Librg.classes import Address

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
dll = ctypes.cdll.LoadLibrary(os.path.join(BASE_DIR, "bin/librg.dll"))


class Interface:
    """Any and all functions which require extra steps other than directly calling the dll can be placed in here"""

    @staticmethod
    def network_start(ctx, address: bytes, port: int):
        address = Address(port, address)
        return bool(dll.librg_network_start(ctx, address))


def dll_func(func, cast=bool):
    """Returns a function wrapper which auto casts DLL returns to specified types
    Arguments:
        func(callable): The function to be wrapped. Must exist in the dll
        cast(callable): The type to be casted, technically could be any callable which takes exactly one argument.
    Returns:
        Function wrapper casting a dll function to the type given in cast.
        """

    def predicate(*args, **kwargs):
        return cast(dll.__getattr__(func)(*args, **kwargs))

    return predicate


# func_map: Dict[Union[Tuple[str, Type], Callable]]
# Dict with value of a Tuple[str, Type], str referencing a librg function and Type being the return type
# or Callable, pointing to a wrapper func in Interface
func_map = {
    "init": ("librg_init",),
    "is_client": ("librg_is_client",),
    "is_connected": ("librg_is_connected",),
    "event_add": ("librg_event_add",),
    "tick": ("librg_tick",)
}
func_map.update(  # Populate func_map with the functions in Interface.
    {func: getattr(Interface, func) for func in dir(Interface) if
     callable(getattr(Interface, func)) and not func.startswith("__")})

__all__ = tuple(func_map.keys())  # __getattr__ won't be called unless this is populated for some reason


def __getattr__(item):
    """Ping the func map, if an attrib is not registered, fallback to the dll"""
    try:
        res = func_map[item]
    except KeyError:
        return dll.__getattr__(item)
    else:
        if callable(res):
            return res  # Return methods from interface.
        else:
            return dll_func(*res)  # Generate wrapper func TODO: Would it be better to generate at init?
