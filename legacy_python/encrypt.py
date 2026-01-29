def rolling_xor_encrypt(text, key_bytes):
    encrypted = []
    text_bytes = text.encode('utf-8')
    key_len = len(key_bytes)
    
    for i in range(len(text_bytes)):
        # XOR the character with the corresponding key byte (rolling)
        encrypted_byte = text_bytes[i] ^ key_bytes[i % key_len]
        # Additional mix: XOR with the previous encrypted byte (CBC-like) to diffuse patterns
        if i > 0:
            encrypted_byte ^= encrypted[i-1]
        
        encrypted.append(encrypted_byte)
        
    return bytes(encrypted).hex().upper()

# This is our new strong key - we will also put this in C++
# "ANTIGRAV_STRONG_KEY_2026_V2"
SECRET_KEY = [0x41, 0x4E, 0x54, 0x49, 0x47, 0x52, 0x41, 0x56, 0x5F, 0x53, 0x54, 0x52, 0x4F, 0x4E, 0x47, 0x5F, 0x4B, 0x45, 0x59, 0x5F, 0x32, 0x30, 0x32, 0x36, 0x5F, 0x56, 0x32]

firebase_url = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
update_url = "https://raw.githubusercontent.com/SKYJ0/GOL_EVENTS_PROV2.1/main/update.json"

with open('encrypted_output.txt', 'w') as f:
    f.write(f"FIREBASE_ENC:{rolling_xor_encrypt(firebase_url, SECRET_KEY)}\n")
    f.write(f"UPDATE_ENC:{rolling_xor_encrypt(update_url, SECRET_KEY)}\n")
