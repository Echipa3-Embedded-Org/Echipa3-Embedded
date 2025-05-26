# Dockerfile pentru audit de securitate

FROM python:3.11-slim

# Setări de bază
WORKDIR /audit-lab
ENV DEBIAN_FRONTEND=noninteractive

# Instalare dependințe de bază
RUN apt-get update && apt-get install -y \
    git curl unzip wget gnupg lsb-release \
    && rm -rf /var/lib/apt/lists/*

# Instalare Bandit (analiză Python)
RUN pip install bandit

# Instalare Semgrep
RUN pip install semgrep

# Instalare Trivy (scanner containere & deps)
RUN wget https://github.com/aquasecurity/trivy/releases/latest/download/trivy_0.50.1_Linux-64bit.deb && \
    dpkg -i trivy_0.50.1_Linux-64bit.deb && rm trivy_0.50.1_Linux-64bit.deb

# Instalare aplicație de test vulnerabilă (ex: Flask)
COPY vulnerable_app/ ./vulnerable_app
WORKDIR /audit-lab/vulnerable_app
RUN pip install -r requirements.txt

# Comanda implicită
CMD ["python", "app.py"]
