#!/usr/bin/env python3

from casual.server.api import call, service, start_server
from casual.server.log import userlog


@service(name = "casual/python/example/upper")
def py_service_uppercase(input : str) -> str:
    """
    Implements uppercase
    """
    userlog(input)
    return input.upper()


@service(name = "casual/python/example/echo")
def py_service_echo(input : str) -> str:
    """
    Implements echo
    """

    userlog(input)
    return input

@service(name = "casual/python/example/hello")
def py_service_hello(input : dict) -> dict:
    """
    Implements hello
    """
    name = input.get("name")

    reply = call("casual/example/echo", name)

    userlog(reply)

    output = {"mesage" : f"Hello {reply}"}
    return output


def main():

    start_server()


if __name__ == "__main__":
    main()
