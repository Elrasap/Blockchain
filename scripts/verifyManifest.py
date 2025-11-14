#!/usr/bin/env python3
import json, sys, hashlib, base64, binascii
from nacl.signing import VerifyKey
from nacl.exceptions import BadSignatureError
from pathlib import Path

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()

def hex_to_bytes(h: str) -> bytes:
    return binascii.unhexlify(h.strip())

def verify_manifest(manifest_path: str, pubkey_path: str) -> bool:
    try:
        manifest = json.loads(Path(manifest_path).read_text())
    except Exception as e:
        print(f"[!] Could not read manifest: {e}")
        return False

    sig_hex = manifest.pop("signature", None)
    if not sig_hex:
        print("[!] No signature field in manifest.")
        return False
    sig = hex_to_bytes(sig_hex)

    # reconstruct message
    msg = json.dumps(manifest, separators=(",", ":"), sort_keys=False).encode()

    try:
        pubkey_bytes = Path(pubkey_path).read_bytes()
        vk = VerifyKey(pubkey_bytes)
        vk.verify(msg, sig)
        print("[OK] Signature valid.")
    except (BadSignatureError, Exception) as e:
        print(f"[!] Signature verification failed: {e}")
        return False

    def verify_file(key_file, key_hash):
        if key_file not in manifest or key_hash not in manifest:
            return True
        file_path = Path(manifest[key_file])
        if not file_path.exists():
            print(f"[!] Missing file {file_path}")
            return False
        expected = manifest[key_hash]
        actual = sha256_file(file_path)
        ok = expected == actual
        print(f"[{'OK' if ok else '!!'}] {file_path} hash "
              f"{'matches' if ok else 'differs'}")
        if not ok:
            print(f"     expected {expected}\n     got      {actual}")
        return ok

    ok = True
    ok &= verify_file("binary", "binary_sha256")
    ok &= verify_file("sbom", "sbom_sha256")
    ok &= verify_file("attestation", "attestation_sha256")
    print("✅ Verification successful." if ok else "❌ Verification failed.")
    return ok

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: verify_manifest.py <release_manifest.json> <public.key>")
        sys.exit(1)
    success = verify_manifest(sys.argv[1], sys.argv[2])
    sys.exit(0 if success else 2)

