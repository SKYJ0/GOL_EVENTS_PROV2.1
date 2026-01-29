import json
import random
import string

def generate_key(prefix="GOL-", length=16):
    chars = string.ascii_uppercase + string.digits
    # Generate 16 random chars
    random_part = ''.join(random.choices(chars, k=length))
    # Split into chunks of 4 for readability: XXXX-XXXX-XXXX-XXXX
    chunks = [random_part[i:i+4] for i in range(0, len(random_part), 4)]
    # Connect prefix with dash: GOL-XXXX-XXXX...
    return prefix + "-" + '-'.join(chunks)

# Define the structure
data = {
    "keys": {}
}

# 1. Admin Key
admin_key = generate_key("GOL")
data["keys"][admin_key] = {
    "name": "Super Admin",
    "role": "admin",
    "status": "active",
    "expiry": "Lifetime",
    "credits": 999999,
    "hwid": ""
}

# 2. Pro Keys (Generates 10)
for i in range(10):
    key = generate_key("GOL")
    data["keys"][key] = {
        "name": f"Pro User {i+1}",
        "role": "pro",
        "status": "active",
        "expiry": "2027-01-01",
        "credits": 0,
        "hwid": ""
    }

# 3. Trial Keys (Generates 5)
for i in range(5):
    key = generate_key("GOL")
    data["keys"][key] = {
        "name": f"Trial User {i+1}",
        "role": "pro",
        "status": "active",
        "expiry": "2026-02-01 12:00",
        "credits": 0,
        "hwid": ""
    }
    
# 4. Free Keys (Generates 5)
for i in range(5):
    key = generate_key("GOL")
    data["keys"][key] = {
        "name": f"Free User {i+1}",
        "role": "user",
        "status": "active",
        "expiry": "Lifetime",
        "credits": 0,
        "hwid": ""
    }

# Save to keys.JSON
with open('keys.JSON', 'w') as f:
    json.dump(data, f, indent=2)

print("Keys updated successfully.")
