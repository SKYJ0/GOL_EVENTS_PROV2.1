def encrypt(plain_text):
    key_str = "ANTIGRAV_STRONG_KEY_2026_V2"
    key = [ord(c) for c in key_str]
    key_len = len(key)
    
    input_bytes = plain_text.encode('utf-8')
    encrypted = bytearray()
    
    prev = 0
    for i in range(len(input_bytes)):
        # C[i] = P[i] ^ K[i] ^ C[i-1]
        k = key[i % key_len]
        p = input_bytes[i]
        
        c = p ^ k ^ prev
        encrypted.append(c)
        prev = c
        
    return encrypted.hex().upper()

secret = "https://raw.githubusercontent.com/SKYJ0/GOLEVENTS_PRO/main/update.json"
print(f"Secret: {secret}")
print(f"Encrypted: {encrypt(secret)}")
