# Proiect Embedded ESP32 – Laborator 9: Securitate

Acest repository conține codul sursă și configurațiile de testare statică pentru proiectul ESP32 realizat în cadrul laboratorului 9.

## 🔐 Obiective Securitate

- Identificarea suprafeței de atac (porturi, credențiale, topicuri MQTT)
- Reducerea riscurilor prin autentificare, validare și rate limiting
- Securizarea imaginii containerelor Docker (ex: Mosquitto)
- Integrare testare statică și scanare de vulnerabilități în CI/CD

## ⚙️ Tool-uri utilizate

- [`cppcheck`](http://cppcheck.sourceforge.net/) – analiză statică cod C/C++
- [`arduino-lint`](https://arduino.github.io/arduino-lint/) – verificare sintaxă Arduino
- [`Trivy`](https://github.com/aquasecurity/trivy) – scanare vulnerabilități în containere
- `GitHub Actions` + `Dependabot` – CI/CD și update automat biblioteci

## 🛠️ Rulare testare locală

```bash
# Analiză statică C++
cppcheck --enable=all --inconclusive --std=c++11 --language=c++ ./src/*.ino

# Lint pentru cod Arduino
arduino-lint --path ./src
