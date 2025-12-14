#!/usr/bin/env python3
import json, sys, hashlib, nacl.signing

def sha256_file(path):
    h = hashlib.sha256()
    with open(path,"rb") as f:
        while True:
            b = f.read(65536)
            if not b: break
            h.update(b)
    return h.hexdigest()

def main():
    try:
        with open("release_manifest.json") as f:
            man = json.load(f)
    except:
        print("Missing release_manifest.json")
        sys.exit(1)

    exe = man.get("binary")
    sig_hex = man.get("signature")
    if not exe or not sig_hex:
        print("Manifest missing binary or signature")
        sys.exit(1)

    digest = sha256_file(exe)
    if digest != man["sha256"]:
        print("SHA256 mismatch")
        sys.exit(1)

    pub = bytes.fromhex(man["pubkey"])
    sig = bytes.fromhex(sig_hex)

    try:
        nacl.signing.VerifyKey(pub).verify(digest.encode(), sig)
    except Exception:
        print("Signature invalid")
        sys.exit(1)

    print("Manifest OK")
    sys.exit(0)

if __name__ == "__main__":
    main()
