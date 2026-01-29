import os
import re
import datetime
from pathlib import Path
from collections import defaultdict
import customtkinter as ctk
import requests
import subprocess
import sys
from tkinter import messagebox

# ================== [ SECURITY CONFIG ] ==================
# تأكد أن هاد الروابط هي نفسها اللي فـ الـ Hub الرئيسي
FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
ACCENT_COLOR = "#1f6aa5"
BG_COLOR = "#0b0b0b"
# =========================================================

def get_hwid():
    try:
        cmd = 'wmic baseboard get serialnumber'
        hwid = str(subprocess.check_output(cmd, shell=True), 'utf-8').split('\n')[1].strip()
        return hwid
    except:
        import hashlib
        return hashlib.sha256(os.environ['COMPUTERNAME'].encode()).hexdigest()[:16]

# --- SECURITY WRAPPER CLASS ---
class SecurityCheck(ctk.CTk):
    def __init__(self, key):
        super().__init__()
        self.withdraw() # مخفي في الأول
        self.license_key = key
        
        # تصميم الـ Splash Screen مطابق للـ Hub
        self.splash = ctk.CTkToplevel(self)
        self.splash.title("Authenticating...")
        self.splash.geometry("450x250")
        self.splash.configure(fg_color=BG_COLOR)
        self.splash.overrideredirect(True) # يحيد إطار النافذة
        
        # وضع النافذة في الوسط
        screen_w = self.winfo_screenwidth()
        screen_h = self.winfo_screenheight()
        x = (screen_w // 2) - 225
        y = (screen_h // 2) - 125
        self.splash.geometry(f"+{x}+{y}")

        ctk.CTkLabel(self.splash, text="GOLEVENTS SECURE 👑", font=("Arial", 24, "bold"), text_color=ACCENT_COLOR).pack(pady=(45, 5))
        self.status_lbl = ctk.CTkLabel(self.splash, text="Verifying Module Access...", font=("Arial", 13), text_color="gray")
        self.status_lbl.pack()
        
        self.progress = ctk.CTkProgressBar(self.splash, width=350, height=12, progress_color=ACCENT_COLOR)
        self.progress.set(0)
        self.progress.pack(pady=30)
        
        # البدء في التحقق بعد 500ms
        self.after(500, self.verify)

    def verify(self):
        hwid = get_hwid()
        self.progress.set(0.4)
        self.update()
        
        try:
            # التحقق من Firebase
            res = requests.get(f"{FIREBASE_URL}/keys/{self.license_key}.json").json()
            if res and res.get("status") == "active" and res.get("hwid") == hwid:
                self.progress.set(1.0)
                self.status_lbl.configure(text="ACCESS GRANTED", text_color="#2ecc71")
                self.update()
                self.after(800, self.launch_main_app)
            else:
                messagebox.showerror("Security Alert", "Unauthorized Access!\nThis module is protected by GOLEVENTS.")
                sys.exit()
        except Exception as e:
            messagebox.showerror("Error", "Could not connect to security server.")
            sys.exit()

    def launch_main_app(self):
        self.splash.destroy()
        self.deiconify() # إظهار النافذة الرئيسية
        # البدء في بناء واجهة السكريبت الأصلي
        GolStockPro(self)

# ============================================================
# --- ORIGINAL LOGIC ENGINE (Calcul_stock) ---
# ============================================================

def parse_seat_detailed(seat_str):
    clean_seat = re.sub(r'(?i)ticket|\.pdf|[^\d[A-Za-z]]', '', str(seat_str))
    match = re.match(r'(\d+)([A-Za-z]*)', clean_seat)
    if match: return int(match.group(1)), match.group(2)
    return 0, str(seat_str)

def extract_srs_from_filename(filename):
    stem = Path(filename).stem
    clean_stem = re.sub(r'(?i)-FV.*', '', stem)
    parts = re.split(r'[- ]', clean_stem)
    parts = [p.strip() for p in parts if p.strip()]
    if len(parts) >= 3:
        return parts[0].upper(), parts[1], parts[2]
    return None, None, None

def extract_fv_from_filename(filename):
    match = re.search(r'FV(\d+(?:p\d+)?)', filename, re.IGNORECASE)
    if match:
        val = match.group(1).replace('p', '.')
        return f"{val}€"
    return "N/A"

def find_consecutive_groups(seat_data_list, step=1):
    if not seat_data_list: return []
    sorted_list = sorted(seat_data_list, key=lambda x: (x['suffix'], x['num']))
    final_groups = []
    curr_group = [sorted_list[0]]
    for i in range(1, len(sorted_list)):
        prev, curr = sorted_list[i-1], sorted_list[i]
        if curr['num'] == prev['num'] + step and curr['suffix'] == prev['suffix']:
            curr_group.append(curr)
        else:
            final_groups.append(curr_group)
            curr_group = [curr]
    final_groups.append(curr_group)
    return final_groups

# --- ORIGINAL UI CLASS (Integrated) ---
class GolStockPro:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - STOCK CALC 2026")
        self.master.geometry("1100x900")
        self.master.configure(fg_color="#0A0C12")
        self.setup_ui()

    def setup_ui(self):
        ctk.CTkLabel(self.master, text="📊 STOCK CALCULATOR PRO", font=("Segoe UI", 32, "bold"), text_color="#00D1FF").pack(pady=(30, 10))
        
        path_frame = ctk.CTkFrame(self.master, fg_color="#1A1D29", corner_radius=15)
        path_frame.pack(fill="x", padx=40, pady=10)
        
        self.path_var = ctk.StringVar()
        ctk.CTkEntry(path_frame, textvariable=self.path_var, width=750, height=45).grid(row=0, column=0, padx=20, pady=20)
        ctk.CTkButton(path_frame, text="BROWSE EVENT", width=150, height=45, fg_color="#00D1FF", text_color="#000", font=("Segoe UI", 13, "bold"), command=self.browse).grid(row=0, column=1, padx=10)

        self.fio_mode = ctk.CTkCheckBox(self.master, text="Odd-Even Mode (ex: Fiorentina events)", font=("Segoe UI", 15), text_color="#AAB")
        self.fio_mode.pack(pady=5)

        ctk.CTkButton(self.master, text="🚀 GENERATE & SAVE REPORT", height=60, width=400, fg_color="#00FF94", text_color="#000", font=("Segoe UI", 18, "bold"), command=self.run_logic).pack(pady=15)

        self.log_area = ctk.CTkTextbox(self.master, fg_color="#05070B", text_color="#00FF94", font=("Consolas", 14), border_width=1, border_color="#1A1D29")
        self.log_area.pack(fill="both", expand=True, padx=40, pady=(0, 30))

    def browse(self):
        p = ctk.filedialog.askdirectory()
        if p: self.path_var.set(p)

    def log(self, msg, clear=False):
        self.log_area.configure(state="normal")
        if clear: self.log_area.delete("1.0", "end")
        self.log_area.insert("end", f"{msg}\n")
        self.log_area.see("end")
        self.log_area.configure(state="disabled")

    def run_logic(self):
        base_path = self.path_var.get()
        if not base_path: return self.log("❌ ERROR: Select folder.")
        
        t_path = os.path.join(base_path, "- Tickets -")
        if not os.path.exists(t_path): return self.log("❌ ERROR: '- Tickets -' folder not found.")

        desktop = Path.home() / "Desktop"
        stock_folder = desktop / "Stock Files"
        stock_folder.mkdir(exist_ok=True)
        
        event_name = os.path.basename(base_path)
        safe_name = re.sub(r'[\\/*?:"<>|]', "", event_name)
        save_file = stock_folder / f"Stock_{safe_name}.txt"

        self.log("⏳ Processing...", clear=True)
        step = 2 if self.fio_mode.get() else 1
        report_body = []
        grand_total = 0

        items_to_process = []
        loose_pdfs = [f for f in os.listdir(t_path) if f.lower().endswith(".pdf") and os.path.isfile(os.path.join(t_path, f))]
        if loose_pdfs: items_to_process.append(("- Extra without folder -", loose_pdfs, t_path))

        for cat in sorted(os.listdir(t_path)):
            cat_path = os.path.join(t_path, cat)
            if os.path.isdir(cat_path):
                pdfs = [f for f in os.listdir(cat_path) if f.lower().endswith(".pdf")]
                if pdfs: items_to_process.append((cat, pdfs, cat_path))

        for cat_name, pdf_list, folder_path in items_to_process:
            report_body.append(f"📂 {cat_name}")
            report_body.append(f"🥅 TOTAL: {len(pdf_list)}")
            grand_total += len(pdf_list)

            data = defaultdict(lambda: defaultdict(list))
            prices = {}
            for f in pdf_list:
                sec, row, seat = extract_srs_from_filename(f)
                if sec and row and seat:
                    n, s = parse_seat_detailed(seat)
                    data[sec][row].append({'raw': str(n)+s, 'num': n, 'suffix': s})
                    if (sec, row) not in prices: prices[(sec, row)] = extract_fv_from_filename(f)

            for sec in sorted(data.keys()):
                sec_groups_info = [] 
                sec_counts = []      
                sorted_rows = sorted(data[sec].keys(), key=lambda x: int(x) if x.isdigit() else x)
                for row in sorted_rows:
                    groups = find_consecutive_groups(data[sec][row], step)
                    for g in groups:
                        qty = len(g)
                        sec_counts.append(str(qty))
                        seat_range = f"{g[0]['raw']}/{g[-1]['raw']}" if qty > 1 else f"{g[0]['raw']}"
                        price = prices.get((sec, row), "N/A")
                        sec_groups_info.append(f"💺Row: {row} Seat: {seat_range} Qty: {qty} [{price}]")

                breakdown = "+".join(sec_counts)
                total_sec = sum(int(c) for c in sec_counts)
                report_body.append(f"🎫 Sector: {sec} Total: {total_sec} | {breakdown}")
                report_body.extend(sec_groups_info)
            report_body.append("")

        header = [f"⚽ EVENT: {event_name}", f"📅 DATE: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M')}", f"🥅 GRAND TOTAL: {grand_total}", "========================================\n"]
        final_report = "\n".join(header) + "\n".join(report_body)
        with open(save_file, "w", encoding="utf-8") as f: f.write(final_report)
        self.log(final_report, clear=True)
        self.log(f"\n✅ SUCCESS!")

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    # كياخد الـ Key من الـ Hub كأول باراميتر
    if len(sys.argv) > 1:
        user_key = sys.argv[1]
        app = SecurityCheck(user_key)
        app.mainloop()
    else:
        # إيلا تحل بوحدو بلا Hub كيعطي هاد الميساج
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk()
        root.withdraw()
        messagebox.showerror("Unauthorized Access", "Please launch this tool through the main GOLEVENTS Hub.")
        sys.exit()