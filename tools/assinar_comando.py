import hmac, hashlib, time, sys

CHAVE = "cole_a_chave_gerada_aqui"

def assinar(cmd):
    ts = int(time.time())
    payload = f"{cmd}|{ts}"
    sig = hmac.new(CHAVE.encode(), payload.encode(), hashlib.sha256).hexdigest().upper()
    return f"{cmd}|{ts}|{sig}"

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python assinar_comando.py <COMANDO>")
        sys.exit(1)
    print(assinar(sys.argv[1]))