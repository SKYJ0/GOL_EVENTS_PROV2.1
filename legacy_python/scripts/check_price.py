import customtkinter as ctk
import requests
import subprocess
import os
import sys
from tkinter import messagebox, filedialog
from bs4 import BeautifulSoup

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "2.9"
FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
ACCENT_COLOR = "#1f6aa5" 
BG_COLOR = "#0b0b0b"
CARD_BG = "#161616"
CARD_BORDER = "#252525"
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
        TicketMarketApp(self)

# --- ORIGINAL TOOL LOGIC (Integrated with CTK) ---
def generate_report(html_content):
    soup = BeautifulSoup(html_content, 'html.parser')
    report_text = ""
    
    match_name_el = soup.find('div', class_='h l m0 mbxs cMag2 p0')
    if not match_name_el:
        return "Error: Match name not found. Make sure to copy the full market data."
    match_name = match_name_el.get_text(strip=True)
    
    all_rows = soup.select('#marketListingsGrid table tbody tr')
    owned_rows = soup.find_all('tr', class_='owned')
    
    if not owned_rows:
        return f"Match: {match_name}\nNo 'owned' listings found."

    for row in owned_rows:
        tds_owned = row.find_all('td')
        section = tds_owned[1].get_text(strip=True)
        our_qty = tds_owned[3].get_text(strip=True)
        our_price = tds_owned[4].get_text(strip=True).replace('€', '').replace(',', '').strip()
        
        our_area_el = row.find('div', class_='ins')
        our_area = our_area_el.get_text(strip=True) if our_area_el else "N/A"
        
        comp_price = "N/A"
        comp_qty = "0"
        
        for r in all_rows:
            tds_comp = r.find_all('td')
            if len(tds_comp) >= 7:
                c_qty = tds_comp[3].get_text(strip=True)
                c_area_el = r.find('div', class_='ins')
                c_area = c_area_el.get_text(strip=True) if c_area_el else ""
                
                if c_area == our_area and c_qty != "1" and c_qty != "":
                    comp_price = tds_comp[4].get_text(strip=True).replace('€', '').replace(',', '').strip()
                    comp_qty = c_qty
                    break
        
        if comp_price == our_price:
            comp_line = "FIRST"
        else:
            comp_line = f"Comp Price = {comp_price} € x{comp_qty}"

        report_text += f"{match_name}\n{our_area}\n{section}\n{comp_line}\nOur Price = {our_price} € x{our_qty}\n"
        report_text += "-" * 35 + "\n"
        
    return report_text

class TicketMarketApp:
    def __init__(self, master):
        self.master = master
        self.master.title("TICKET MARKET PARSER PRO 👑")
        self.master.geometry("800x700")
        self.master.configure(fg_color=BG_COLOR)

        # Header Section
        header_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        header_frame.pack(fill="x", padx=30, pady=30)
        
        ctk.CTkLabel(header_frame, text="🔍 LISTING CHECKER PRO", font=("Arial", 28, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header_frame, text="Select an HTML file to compare market prices.", font=("Arial", 14), text_color="gray60").pack(anchor="w")

        # Main Action Frame
        action_frame = ctk.CTkFrame(self.master, fg_color=CARD_BG, corner_radius=15, border_width=1, border_color=CARD_BORDER)
        action_frame.pack(fill="x", padx=30, pady=10)

        self.btn_open = ctk.CTkButton(action_frame, text="📁 BROWSE HTML FILE", font=("Arial", 14, "bold"), 
                                      fg_color=ACCENT_COLOR, height=50, command=self.open_file)
        self.btn_open.pack(pady=20, padx=20, fill="x")

        # Results Area
        self.result_area = ctk.CTkTextbox(self.master, font=("Consolas", 13), fg_color="#050505", 
                                          text_color="#00FF94", border_width=1, border_color=CARD_BORDER)
        self.result_area.pack(fill="both", expand=True, padx=30, pady=20)

        # Footer Actions
        footer_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        footer_frame.pack(fill="x", padx=30, pady=(0, 30))

        self.btn_copy = ctk.CTkButton(footer_frame, text="📋 COPY REPORT", font=("Arial", 14, "bold"),
                                      fg_color="#2ecc71", hover_color="#27ae60", text_color="black",
                                      height=45, command=self.copy_to_clipboard)
        self.btn_copy.pack(side="left", fill="x", expand=True, padx=(0, 10))

        self.btn_clear = ctk.CTkButton(footer_frame, text="🗑️ CLEAR", font=("Arial", 14, "bold"),
                                       fg_color="#e74c3c", hover_color="#c0392b", height=45, command=self.clear_all)
        self.btn_clear.pack(side="left", fill="x", expand=True)

    def open_file(self):
        file_path = filedialog.askopenfilename(filetypes=[("HTML files", "*.html")])
        if file_path:
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()
                    report = generate_report(content)
                    self.result_area.delete(1.0, "end")
                    self.result_area.insert("end", report)
            except Exception as e:
                messagebox.showerror("Error", f"Could not read file: {e}")

    def clear_all(self):
        self.result_area.delete(1.0, "end")

    def copy_to_clipboard(self):
        result = self.result_area.get(1.0, "end").strip()
        if result:
            self.master.clipboard_clear()
            self.master.clipboard_append(result)
            messagebox.showinfo("Success", "Report copied to clipboard!")

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        # Emergency block if opened without Hub
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk()
        root.withdraw()
        messagebox.showerror("Unauthorized", "Please launch via GOLEVENTS Hub.")
        sys.exit()