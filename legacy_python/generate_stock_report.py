import os
import re
import sys
import collections

def normalize_name(name):
    return name.strip().lower()

def classify_platform(folder_name):
    # Gogo: 9 digits (e.g. 626946288)
    if re.search(r'\b\d{9}\b', folder_name):
        return "Gogo"
    # Tixstock: 7 digits (e.g. 1539879, 1570798) -> User says this is "Net"
    if re.search(r'\b\d{7}\b', folder_name):
        return "Net"
    # Tixstock: 8 char alphanumeric (e.g. BBC7F524)
    if re.search(r'\b[A-F0-9]{8}\b', folder_name, re.IGNORECASE):
        return "Tixstock"
    return "Other Source"

def count_pdfs(folder_path):
    count = 0
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.lower().endswith(".pdf"):
                count += 1
    return count

def parse_sector_and_quantity(folder_name, full_path=None):
    qty = 0
    qty_match = re.search(r'x\s*(\d+)', folder_name, re.IGNORECASE)
    if qty_match:
        qty = int(qty_match.group(1))
    elif full_path:
        qty = count_pdfs(full_path)
    
    if qty == 0: 
        qty = 1

    sector = folder_name
    
    # Priority to IDs
    # 9 digits (Gogo)
    id_match = re.search(r'^(.*?)\s+\d{9}', folder_name)
    if not id_match:
        # 7 digits (Net)
        id_match = re.search(r'^(.*?)\s+\d{7}', folder_name)
    if not id_match:
        # 8 chars Hex (Tixstock)
        id_match = re.search(r'^(.*?)\s+[A-F0-9]{8}\b', folder_name, re.IGNORECASE)
    
    if id_match:
        sector_cand = id_match.group(1).strip()
        sector_cand = re.sub(r' ID-$', '', sector_cand, flags=re.IGNORECASE).strip()
        if sector_cand:
            sector = sector_cand
    else:
        # Try simple "xN" split if no ID found
        if qty_match:
            # We want part before xN
            pre_x = folder_name[:qty_match.start()].strip()
            if pre_x:
                sector = pre_x

    sector = sector.strip(' -')
    return sector, qty

def scan_stock(root_path):
    stock_map = collections.defaultdict(lambda: {'count': 0, 'files': set()})
    
    tickets_path = os.path.join(root_path, "- Tickets -")
    if not os.path.exists(tickets_path):
        tickets_path = os.path.join(root_path, "tickets")
        if not os.path.exists(tickets_path):
            print(f"Warning: '- Tickets -' folder not found. Scanning root.")
            tickets_path = root_path
    
    print(f"Scanning stock in: {tickets_path}")
    
    seat_re = re.compile(r"^([A-Za-z0-9]+)-(\d+)-([A-Za-z0-9]+)")
    
    for root, dirs, files in os.walk(tickets_path):
        for file in files:
            if not file.lower().endswith(".pdf"):
                continue
                
            match = seat_re.match(file)
            if match:
                sector = match.group(1)
                stock_map[sector]['count'] += 1
                stock_map[sector]['files'].add(file)
            else:
                 pass

    return stock_map

def find_delivered_folder(root_path):
    candidates = ["caricati", "carricati"]
    try:
        all_dirs = [d for d in os.listdir(root_path) if os.path.isdir(os.path.join(root_path, d))]
        for d in all_dirs:
            d_lower = d.lower()
            for cand in candidates:
                if cand in d_lower:
                    return d
    except Exception:
        pass
    return None

def scan_delivered(root_path):
    delivered_folder = find_delivered_folder(root_path)
    delivered_map = collections.defaultdict(int)
    
    if delivered_folder:
        full_path = os.path.join(root_path, delivered_folder)
        seat_re = re.compile(r"^([A-Za-z0-9]+)-(\d+)-([A-Za-z0-9]+)")
        for root, dirs, files in os.walk(full_path):
            for file in files:
                if file.lower().endswith(".pdf"):
                     match = seat_re.match(file)
                     if match:
                         delivered_map[match.group(1)] += 1
                     else:
                         delivered_map['Unknown'] += 1
    
    return delivered_map, delivered_folder

def scan_delivered_orders(root_path, delivered_folder):
    orders = []
    if not delivered_folder: 
        return orders
        
    full_path = os.path.join(root_path, delivered_folder)
    try:
        entries = os.listdir(full_path)
        for entry in entries:
            entry_path = os.path.join(full_path, entry)
            if os.path.isdir(entry_path):
                sec, qty = parse_sector_and_quantity(entry, entry_path)
                if sec:
                    orders.append({
                        'folder': entry,
                        'sector': sec,
                        'quantity': qty,
                        'is_pending': False 
                    })
    except:
        pass
    return orders

def scan_orders(root_path, delivered_folder):
    orders = []
    
    try:
        entries = os.listdir(root_path)
    except:
        return orders

    for entry in entries:
        full_path = os.path.join(root_path, entry)
        if not os.path.isdir(full_path):
            continue
            
        if "-" in entry:
            continue
            
        if delivered_folder and delivered_folder.lower() == entry.lower():
             continue

        sec, qty = parse_sector_and_quantity(entry, full_path)
             
        orders.append({
            'folder': entry,
            'sector': sec,
            'quantity': qty,
            'is_pending': True
        })
            
    return orders

def resolve_sector(order_sector, available_sectors):
    if order_sector in available_sectors:
        return order_sector
    for s in available_sectors:
        if s.lower() == order_sector.lower():
            return s
    for s in available_sectors:
        if s.lower() in order_sector.lower():
            return s
    return None

def get_mapping_from_user(order_folder, available_sectors, stock_map):
    print(f"\n[?] Order '{order_folder}' does not match any known stock sector.")
    print("    Do you want to map it to a specific sector? (y/n): ", end='', flush=True)
    try:
        choice = sys.stdin.readline().strip().lower()
    except:
        return None
    
    if choice == 'y':
        print("    Available Sectors:")
        sorted_secs = sorted(available_sectors)
        for i, sec in enumerate(sorted_secs):
            qty = stock_map.get(sec, 0)
            print(f"    {i+1}. {sec} (Qty: {qty})")
        
        print("    Select sector number (or 0 to cancel): ", end='', flush=True)
        try:
            line = sys.stdin.readline().strip()
            if not line: return None
            sel = int(line)
            if 1 <= sel <= len(sorted_secs):
                return sorted_secs[sel-1]
        except ValueError:
            pass
            
    print("    -> Ignoring/Marking as unknown.")
    return None

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_stock_report.py <folder_path>")
        sys.exit(1)
        
    root_path = sys.argv[1]
    if not os.path.exists(root_path):
        print(f"Error: Path '{root_path}' does not exist.")
        sys.exit(1)

    output_lines = []
    def log(msg):
        print(msg)
        output_lines.append(str(msg))

    folder_title = os.path.basename(os.path.normpath(root_path))
    log(f"--- 🏟️ {folder_title} ---")
    
    # 1. Scan Stock (silent)
    stock_map = scan_stock(root_path)

    # 2. Scan Delivered (silent)
    delivered_map, delivered_folder_name = scan_delivered(root_path)

    # 3. Scan Orders (silent)
    orders = scan_orders(root_path, delivered_folder_name)
    delivered_order_folders = scan_delivered_orders(root_path, delivered_folder_name)
    
    reserved_per_sector = collections.defaultdict(int)
    notes = []
    
    available_sectors = list(stock_map.keys())
    remaining_stock = {k: v['count'] for k, v in stock_map.items()}

    total_ignored_qty = 0

    # Map Pending Orders (silent)
    for order in orders:
        real_sector = resolve_sector(order['sector'], available_sectors)
        
        if not real_sector:
            real_sector = get_mapping_from_user(order['folder'], available_sectors, remaining_stock)
        
        if real_sector:
            order['resolved_sector'] = real_sector
            reserved_per_sector[real_sector] += order['quantity']
            if real_sector in remaining_stock:
                remaining_stock[real_sector] -= order['quantity']
        else:
            order['resolved_sector'] = order['sector'] 
            total_ignored_qty += order['quantity']
            notes.append(f"Folder '{order['folder']}' (x{order['quantity']}) ignored or not in stock.")


    # 4. Platform Stats
    log("\n--- 🛒 PLATFORM ORDERS SUMMARY ---")
    platform_data = collections.defaultdict(lambda: collections.defaultdict(int))
    
    for o in orders:
        plat = classify_platform(o['folder'])
        sec = o.get('resolved_sector', o['sector'])
        platform_data[plat][sec] += o['quantity']

    for o in delivered_order_folders:
        plat = classify_platform(o['folder'])
        sec_clean = resolve_sector(o['sector'], available_sectors)
        if not sec_clean:
            sec_clean = o['sector']
        platform_data[plat][sec_clean] += o['quantity']

    log(f"{'PLATFORM':<15} | {'SECTOR':<15} | {'TICKETS':>8}")
    log("-" * 42)
    
    grand_total_tickets = 0
    
    for plat in sorted(platform_data.keys()):
        sectors = platform_data[plat]
        for sec in sorted(sectors.keys()):
            qty = sectors[sec]
            log(f"{plat:<15} | {sec:<15} | {qty:>8}")
            grand_total_tickets += qty
            
    log("-" * 42)
    log(f"{'TOTAL':<33} | {grand_total_tickets:>8}")

    log("\n--- 📈 TOTAL STOCK REPORT ---")
    
    headers = ["SECTOR", "TOTAL", "RESERVED", "DELIVERED"]
    log(f"{headers[0]:<15} | {headers[1]:>8} | {headers[2]:>8} | {headers[3]:>9}")
    log("-" * 50)
    
    all_sectors = set(stock_map.keys()) | set(delivered_map.keys()) | set(reserved_per_sector.keys())
    
    grand_net_stock = 0
    grand_reserved = 0
    grand_delivered = 0
    
    for sector in sorted(all_sectors):
        if sector == "Unknown": continue 
        
        total_stock = stock_map[sector]['count']
        reserved = reserved_per_sector[sector]
        delivered = delivered_map[sector]
        
        net_stock = total_stock - reserved
        
        grand_net_stock += net_stock
        grand_reserved += reserved
        grand_delivered += delivered
        
        log(f"{sector:<15} | {net_stock:>8} | {reserved:>8} | {delivered:>9}")
        
    log("\n--- 📊 FINAL SUMMARY ---")
    log(f"STOCK AVAILABLE: {grand_net_stock}")
    
    # Updated: SOLD label changed, calculation implies ignored are included (User wanted 13 for logic, but clean label)
    grand_sold = grand_reserved + grand_delivered + total_ignored_qty
    log(f"SOLD (Reserved + Delivered ): {grand_sold}")
    if total_ignored_qty > 0:
        log(f"   (Orders without tickets yet: {total_ignored_qty})")
    
    with open("report_final.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(output_lines))
    
if __name__ == "__main__":
    main()
