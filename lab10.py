# auth_audit.py
import requests
from urllib.parse import urljoin

BASE_URL = "http://your-app:8080"

def test_sqli():
    payload = "' OR '1'='1' --"
    res = requests.post(
        urljoin(BASE_URL, "/login"),
        data={"username": payload, "password": "test"}
    )
    if "error in your SQL syntax" in res.text:
        print("[CRITICAL] SQL Injection vulnerability found")

def test_xss():
    payload = "<script>alert(1)</script>"
    res = requests.get(
        urljoin(BASE_URL, f"/search?q={payload}")
    )
    if payload in res.text:
        print("[HIGH] XSS vulnerability found")

if __name__ == "__main__":
    test_sqli()
    test_xss()
