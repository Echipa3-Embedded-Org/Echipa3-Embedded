# fuzz/fuzz_api.py
from boofuzz import *
import requests

def fuzz_target():
    session = Session(
        target=Target(connection=SocketConnection("localhost", 5000, proto='tcp'))
    )

    s_initialize("upload")
    s_static(b"POST /upload HTTP/1.1\r\n")
    s_static(b"Host: localhost\r\n")
    s_static(b"Content-Length: ")
    s_size("payload", output_format="ascii", fuzzable=False)
    s_static(b"\r\nContent-Type: application/octet-stream\r\n\r\n")
    s_string("payload", name="payload", max_len=10000)

    session.connect(s_get("upload"))
    session.fuzz()

if __name__ == "__main__":
    fuzz_target()
