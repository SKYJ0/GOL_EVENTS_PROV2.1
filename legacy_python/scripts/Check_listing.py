import tkinter as tk
from tkinter import messagebox, scrolledtext
import re
import customtkinter as ctk
import requests
import subprocess
import os
import sys

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "2.9"
FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
ACCENT_COLOR = "#1f6aa5" 
BG_COLOR = "#0b0b0b"
CARD_BG = "#161616"
TEXT_COLOR = "#f8fafc"

# Palette for the tool (Matched with Hub)
COLOR_BG = "#0b0b0b"
COLOR_CARD = "#161616"
COLOR_ACCENT = "#1f6aa5"
COLOR_SUCCESS = "#10b981"
COLOR_DANGER = "#f43f5e"
COLOR_GOGO = "#38bdf8"
COLOR_NET = "#fb7185"
COLOR_TIX = "#fbbf24"
# =================================================================

def get_hwid():
    try:
        cmd = 'wmic baseboard get serialnumber'
        hwid = str(subprocess.check_output(cmd, shell=True), 'utf-8').split('\n')[1].strip()
        return hwid
    except:
        import hashlib
        return hashlib.sha256(os.environ['COMPUTERNAME'].encode()).hexdigest()[:16]

# --- SECURITY WRAPPER ---
class SecurityCheck(ctk.CTk):
    def __init__(self, key):
        super().__init__()
        self.withdraw()
        self.license_key = key
        
        self.splash = ctk.CTkToplevel(self)
        self.splash.title("Authenticating Module...")
        self.splash.geometry("450x250")
        self.splash.configure(fg_color=BG_COLOR)
        self.splash.overrideredirect(True)
        
        x = (self.winfo_screenwidth() // 2) - 225
        y = (self.winfo_screenheight() // 2) - 125
        self.splash.geometry(f"+{x}+{y}")

        ctk.CTkLabel(self.splash, text="GOLEVENTS SECURE 👑", font=("Arial", 24, "bold"), text_color=ACCENT_COLOR).pack(pady=(45, 5))
        self.status_lbl = ctk.CTkLabel(self.splash, text="Verifying Module Access...", font=("Arial", 13), text_color="gray")
        self.status_lbl.pack()
        
        self.progress = ctk.CTkProgressBar(self.splash, width=350, height=12, progress_color=ACCENT_COLOR)
        self.progress.set(0)
        self.progress.pack(pady=30)
        
        self.after(500, self.verify)

    def verify(self):
        hwid = get_hwid()
        try:
            res = requests.get(f"{FIREBASE_URL}/keys/{self.license_key}.json").json()
            if res and res.get("status") == "active" and res.get("hwid") == hwid:
                self.progress.set(1.0)
                self.status_lbl.configure(text="ACCESS GRANTED", text_color="#2ecc71")
                self.after(800, self.launch_main_tool)
            else:
                messagebox.showerror("Security Alert", "Unauthorized Access!\nPlease launch through GOLEVENTS Hub.")
                sys.exit()
        except:
            sys.exit()

    def launch_main_tool(self):
        self.splash.destroy()
        self.deiconify()
        StockProDashboard(self)

# --- ORIGINAL TOOL LOGIC (Updated Design) ---
class StockProDashboard:
    def __init__(self, master):
        self.master = master
        self.master.title("STOCK ALLOCATOR PRO v2.9 | GOLEVENTS Edition")
        self.master.geometry("1350x900")
        self.master.configure(bg=COLOR_BG)

        # --- Top Navigation / Status Bar ---
        top_bar = tk.Frame(self.master, bg=COLOR_CARD, height=70)
        top_bar.pack(fill="x", side="top")
        top_bar.pack_propagate(False)

        tk.Label(top_bar, text="STOCK ALLOCATOR PRO 👑", font=("Segoe UI", 16, "bold"), 
                 bg=COLOR_CARD, fg=COLOR_ACCENT).pack(side="left", padx=30)

        self.stats_frame = tk.Frame(top_bar, bg=COLOR_CARD)
        self.stats_frame.pack(side="right", padx=30)
        
        self.lbl_grand = tk.Label(self.stats_frame, text="TOTAL LISTED: 0", font=("Segoe UI", 12, "bold"), 
                                  bg=COLOR_CARD, fg=TEXT_COLOR)
        self.lbl_grand.pack(side="right")

        # --- Main Layout ---
        self.main_frame = tk.Frame(self.master, bg=COLOR_BG)
        self.main_frame.pack(fill="both", expand=True, padx=20, pady=20)

        left_col = tk.Frame(self.main_frame, bg=COLOR_BG)
        left_col.pack(side="left", fill="both", expand=True, padx=(0, 10))

        self.create_section_label(left_col, "1. ORIGINAL STOCK INPUT").pack(anchor="w")
        self.txt_original = scrolledtext.ScrolledText(left_col, height=12, bg=COLOR_CARD, fg=TEXT_COLOR, 
                                                      insertbackground="white", borderwidth=1, relief="flat", font=("Consolas", 10))
        self.txt_original.pack(fill="x", pady=(5, 15))

        self.create_section_label(left_col, "2. UNIFIED LISTING TABLE").pack(anchor="w")
        table_container = tk.Frame(left_col, bg=COLOR_CARD, padx=15, pady=15)
        table_container.pack(fill="both", expand=True)

        h_f = tk.Frame(table_container, bg=COLOR_CARD)
        h_f.pack(fill="x", pady=(0, 10))
        tk.Label(h_f, text="SECTOR", width=12, bg=COLOR_CARD, fg="gray", font=("Segoe UI", 9, "bold")).pack(side="left")
        tk.Label(h_f, text="GOGO QTY", width=20, bg=COLOR_CARD, fg=COLOR_GOGO, font=("Segoe UI", 9, "bold")).pack(side="left", padx=5)
        tk.Label(h_f, text="NET QTY", width=20, bg=COLOR_CARD, fg=COLOR_NET, font=("Segoe UI", 9, "bold")).pack(side="left", padx=5)
        tk.Label(h_f, text="TIX QTY", width=20, bg=COLOR_CARD, fg=COLOR_TIX, font=("Segoe UI", 9, "bold")).pack(side="left", padx=5)

        self.rows_inner = tk.Frame(table_container, bg=COLOR_CARD)
        self.rows_inner.pack(fill="both", expand=True)
        
        self.unified_rows = []
        for _ in range(10): self.add_row()

        tk.Button(table_container, text="+ ADD NEW SECTOR ROW", font=("Segoe UI", 8, "bold"), 
                  bg="#2a2a2a", fg="white", borderwidth=0, padx=15, command=self.add_row).pack(pady=10)

        right_col = tk.Frame(self.main_frame, bg=COLOR_BG)
        right_col.pack(side="right", fill="both", expand=True, padx=(10, 0))

        self.create_section_label(right_col, "3. FINAL PROCESSED RESULT").pack(anchor="w")
        self.txt_result = scrolledtext.ScrolledText(right_col, bg="#050505", fg=COLOR_SUCCESS, 
                                                    insertbackground="white", borderwidth=0, 
                                                    font=("Consolas", 12, "bold"))
        self.txt_result.pack(fill="both", expand=True, pady=(5, 10))

        action_bar = tk.Frame(self.master, bg=COLOR_BG, height=80)
        action_bar.pack(fill="x", side="bottom", pady=20)
        
        self.btn_process = tk.Button(action_bar, text="🚀  GENERATE FINAL STOCK", font=("Segoe UI", 12, "bold"), 
                                     bg=COLOR_ACCENT, fg="white", borderwidth=0, padx=50, pady=15, 
                                     cursor="hand2", command=self.process)
        self.btn_process.pack(side="left", padx=(40, 20))

        self.btn_clear = tk.Button(action_bar, text="🗑️  RESET ALL", font=("Segoe UI", 12, "bold"), 
                                   bg=COLOR_DANGER, fg="white", borderwidth=0, padx=30, pady=15, 
                                   cursor="hand2", command=self.clear_fields)
        self.btn_clear.pack(side="left")

    def create_section_label(self, parent, text):
        return tk.Label(parent, text=text, font=("Segoe UI", 9, "bold"), bg=COLOR_BG, fg="gray60")

    def add_row(self):
        f = tk.Frame(self.rows_inner, bg=COLOR_CARD)
        f.pack(fill="x", pady=2)
        s = tk.Entry(f, width=12, font=("Segoe UI", 10), bg="#0d0d0d", fg="white", borderwidth=0, justify="center")
        s.pack(side="left", padx=2, ipady=3)
        g = tk.Entry(f, width=24, font=("Segoe UI", 10), bg="#0d0d0d", fg=COLOR_GOGO, borderwidth=0)
        g.pack(side="left", padx=5, ipady=3)
        n = tk.Entry(f, width=24, font=("Segoe UI", 10), bg="#0d0d0d", fg=COLOR_NET, borderwidth=0)
        n.pack(side="left", padx=5, ipady=3)
        t = tk.Entry(f, width=24, font=("Segoe UI", 10), bg="#0d0d0d", fg=COLOR_TIX, borderwidth=0)
        t.pack(side="left", padx=5, ipady=3)
        self.unified_rows.append({'sector': s, 'GOGO': g, 'NET': n, 'TIXSTOCK': t})

    def clear_fields(self):
        self.txt_original.delete("1.0", tk.END); self.txt_result.delete("1.0", tk.END)
        for r in self.unified_rows:
            for k in ['sector', 'GOGO', 'NET', 'TIXSTOCK']: r[k].delete(0, tk.END)
        self.lbl_grand.config(text="TOTAL LISTED: 0")

    def show_danger_popup(self, messages):
        pop = tk.Toplevel(self.master); pop.title("ERROR"); pop.geometry("500x400"); pop.configure(bg="#1a0505")
        tk.Label(pop, text="🚨", font=("Arial", 60), bg="#1a0505").pack(pady=10)
        tk.Label(pop, text="OVER-LISTING DANGER", font=("Segoe UI", 14, "bold"), bg="#1a0505", fg="white").pack()
        st = scrolledtext.ScrolledText(pop, height=8, width=50, bg="#000", fg="#ff8888", borderwidth=0)
        st.pack(pady=15, padx=20)
        for m in messages: st.insert(tk.END, f"• {m}\n")
        tk.Button(pop, text="CLOSE", bg="white", command=pop.destroy).pack(pady=10)

    def process(self):
        stock_text = self.txt_original.get("1.0", tk.END).strip()
        if not stock_text: return
        listings = {"GOGO": {}, "NET": {}, "TIXSTOCK": {}}
        grand_sum = 0
        for row in self.unified_rows:
            sec = row['sector'].get().strip().upper()
            if not sec: continue
            for p in ["GOGO", "NET", "TIXSTOCK"]:
                q_s = row[p].get().strip()
                if q_s:
                    qs = [int(n) for n in re.findall(r'\d+', q_s)]
                    if sec not in listings[p]: listings[p][sec] = []
                    listings[p][sec].extend(qs); grand_sum += sum(qs)
        self.lbl_grand.config(text=f"TOTAL LISTED: {grand_sum}")
        row_p = re.compile(r"(.*Row:.*?Qty:\s*(\d+)\s*)\[(.*?)\]", re.IGNORECASE)
        sec_p = re.compile(r"Sector:\s*([A-Za-z0-9]+)", re.IGNORECASE)
        lines = stock_text.split('\n')
        row_reg, sec_cap, curr_s = {}, {}, None
        for i, line in enumerate(lines):
            sm = sec_p.search(line)
            if sm: curr_s = sm.group(1).upper(); sec_cap[curr_s] = sec_cap.get(curr_s, 0)
            rm = row_p.search(line)
            if rm and curr_s:
                qty = int(rm.group(2)); sec_cap[curr_s] += qty
                row_reg[i] = {'sector': curr_s, 'total': qty, 'remaining': qty, 'allocs': [], 'prefix': rm.group(1)}
        danger = []
        all_s = set(list(listings["GOGO"].keys()) + list(listings["NET"].keys()) + list(listings["TIXSTOCK"].keys()))
        for s in all_s:
            tot = sum(listings["GOGO"].get(s, [])) + sum(listings["NET"].get(s, [])) + sum(listings["TIXSTOCK"].get(s, []))
            avail = sec_cap.get(s, 0)
            if tot > avail: danger.append(f"Sector {s}: {tot} vs {avail} available")
        if danger: self.show_danger_popup(danger); return
        demands = []
        for p in ["GOGO", "NET", "TIXSTOCK"]:
            for s, qs in listings[p].items():
                for q in qs: demands.append({'plat': p, 'qty': q, 'sec': s})
        for d in demands:
            for r in row_reg.values():
                if r['sector'] == d['sec'] and r['remaining'] == d['qty']:
                    r['allocs'].append(f"X{d['qty']} {d['plat']}"); r['remaining'] = 0; d['qty'] = 0; break
        for d in demands:
            if d['qty'] <= 0: continue
            for r in row_reg.values():
                if r['sector'] == d['sec'] and r['remaining'] >= d['qty']:
                    r['allocs'].append(f"X{d['qty']} {d['plat']}"); r['remaining'] -= d['qty']; d['qty'] = 0; break
        for d in demands:
            if d['qty'] <= 0: continue
            for r in row_reg.values():
                if r['sector'] == d['sec'] and r['remaining'] > 0:
                    take = min(d['qty'], r['remaining']); r['allocs'].append(f"X{take} {d['plat']}")
                    r['remaining'] -= take; d['qty'] -= take
        res = []
        for i, line in enumerate(lines):
            if i in row_reg:
                reg = row_reg[i]
                if reg['allocs']:
                    a_s = " / ".join(reg['allocs']); s_s = f" STILL X{reg['remaining']}" if reg['remaining'] > 0 else ""
                    res.append(f"{reg['prefix']}[{a_s}]{s_s}")
                else: res.append(line)
            else: res.append(line)
        self.txt_result.delete("1.0", tk.END); self.txt_result.insert(tk.END, "\n".join(res))

# --- EXECUTION ---
if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        # Emergency block
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk()
        root.withdraw()
        messagebox.showerror("Unauthorized", "Launch via GOLEVENTS Hub.")
        sys.exit()