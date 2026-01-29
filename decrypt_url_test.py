
def decrypt(hex_str):
    key_str = "ANTIGRAV_STRONG_KEY_2026_V2"
    key = [ord(c) for c in key_str]
    key_len = len(key)
    
    encrypted = bytearray.fromhex(hex_str)
    decrypted = bytearray()
    
    # Decryption Loop
    # P[i] = C[i] ^ C[i-1] ^ K[i]
    for i in range(len(encrypted)):
        c = encrypted[i]
        k = key[i % key_len]
        prev = encrypted[i-1] if i > 0 else 0
        
        p = c ^ k ^ prev
        decrypted.append(p)
        
    return decrypted.decode('utf-8')

hex_data = "2913330A3E5638416C5E7D01290E3D0A34133F134406570E3F1D4A655F250F27187673676D731171787063777778621E7A1B727D7904136F156D053A1A251468496B406F5C66032C06366A"
print(f"Decrypted URL: {decrypt(hex_data)}")
