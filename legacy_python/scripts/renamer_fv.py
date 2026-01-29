import os
import re
import threading
import customtkinter as ctk
import requests
import subprocess
import sys
from tkinter import messagebox, filedialog
from PyPDF2 import PdfReader

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "2.9"
FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
ACCENT_COLOR = "#1f6aa5" 
PRO_COLOR = "#ffcc00"
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
        self.splash.title("Authenticating...")
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
        TicketRenamerApp(self)

# --- CORE LOGIC: ADVANCED FACE VALUE EXTRACTION ---
def extract_face_value(pdf_path):
    try:
        reader = PdfReader(pdf_path)
        text = ""
        for page in reader.pages:
            page_text = page.extract_text()
            if page_text:
                # Clean text: remove spaces between numbers (e.g., 1 5 0 . 0 0 -> 150.00)
                page_text = re.sub(r'(\d)\s*([.,])\s*(\d)', r'\1\2\3', page_text)
                text += page_text + "\n"

        # Pattern for Prices: X.XX to XXXX.XX
        # Strict format: 1 to 4 digits before dot, exactly 2 digits after.
        price_pattern = r'\b\d{1,4}[.,]\d{2}\b'

        # 1. Target search with Keywords
        keywords = r'(?:TOTALE|TOTAL|IMPORTO|VALORE|PREZZO|PRICE|PRIX|PRECIO)'
        search_pattern = keywords + r'\s*(?:TICKET|AMOUNT|EUR|€)?[:\s€]*(\b\d{1,4}[.,]\d{2}\b)'
        
        matches = re.findall(search_pattern, text, re.IGNORECASE)
        if matches:
            return matches[-1].replace(",", ".") # Return the last found total

        # 2. Fallback: Get the HIGHEST number matching the format
        all_candidates = re.findall(price_pattern, text)
        if all_candidates:
            float_prices = [float(p.replace(",", ".")) for p in all_candidates]
            max_p = max(float_prices)
            return "{:.2f}".format(max_p)

        return None
    except:
        return None

# --- GUI INTERFACE ---
class TicketRenamerApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - TICKET RENAMER PRO 🏷️")
        self.master.geometry("900x750")
        self.master.configure(fg_color=BG_COLOR)

        # Header
        header_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        header_frame.pack(fill="x", padx=40, pady=(30, 10))
        
        ctk.CTkLabel(header_frame, text="🏷️ TICKET RENAMER + FV", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header_frame, text="Auto-detect prices inside PDFs and rename files using strict XX.YY format.", font=("Arial", 13), text_color="gray60").pack(anchor="w")

        # Folder Selection
        self.path_var = ctk.StringVar(value="Click Browse to select folder...")
        path_frame = ctk.CTkFrame(self.master, fg_color=CARD_BG, corner_radius=15, border_width=1, border_color=CARD_BORDER)
        path_frame.pack(fill="x", padx=40, pady=20)

        ctk.CTkEntry(path_frame, textvariable=self.path_var, width=600, height=45, state="readonly", fg_color="#0d0d0d").pack(side="left", padx=20, pady=20)
        ctk.CTkButton(path_frame, text="BROWSE", width=120, height=45, fg_color=ACCENT_COLOR, font=("Arial", 12, "bold"), command=self.browse).pack(side="left", padx=(0, 20))

        # Action Button
        self.run_btn = ctk.CTkButton(self.master, text="🚀 START BULK RENAMING", height=60, 
                                      fg_color="#10b981", hover_color="#059669", text_color="black",
                                      font=("Arial", 16, "bold"), command=self.start_process)
        self.run_btn.pack(pady=10, padx=40, fill="x")

        # Logs
        self.log_area = ctk.CTkTextbox(self.master, height=250, fg_color="#050505", text_color="#00FF88", 
                                        font=("Consolas", 12), border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(fill="both", expand=True, padx=40, pady=20)

    def browse(self):
        p = filedialog.askdirectory()
        if p: self.path_var.set(p)

    def log(self, msg):
        self.log_area.configure(state="normal")
        self.log_area.insert("end", f"> {msg}\n")
        self.log_area.see("end")
        self.log_area.configure(state="disabled")
        self.master.update()

    def start_process(self):
        folder = self.path_var.get()
        if folder == "Click Browse to select folder..." or not os.path.isdir(folder):
            messagebox.showwarning("Warning", "Please select a valid folder first.")
            return

        self.run_btn.configure(state="disabled", text="PROCESSING...")
        threading.Thread(target=self.process_files, args=(folder,), daemon=True).start()

    def process_files(self, folder):
        self.log(f"Scanning folder: {folder}")
        files = [f for f in os.listdir(folder) if f.lower().endswith(".pdf") and "-FV" not in f.upper()]
        
        if not files:
            self.log("No new PDF files found or all files already have FV tags.")
            self.run_btn.configure(state="normal", text="🚀 START BULK RENAMING")
            return

        success_count = 0
        for filename in files:
            old_path = os.path.join(folder, filename)
            price = extract_face_value(old_path)
            
            if price:
                # Format: replace dot with 'p' (e.g., 50.00 -> 50p00)
                clean_price = price.replace(".", "p")
                base_name = os.path.splitext(filename)[0]
                new_filename = f"{base_name}-FV{clean_price}.pdf"
                new_path = os.path.join(folder, new_filename)
                
                try:
                    if not os.path.exists(new_path):
                        os.rename(old_path, new_path)
                        self.log(f"SUCCESS: {filename} ⮕ FV {price}€")
                        success_count += 1
                except Exception as e:
                    self.log(f"ERROR: {filename} | {e}")
            else:
                self.log(f"SKIPPED: Format XX.YY not found in {filename}")

        self.log(f"\n✅ FINISHED! Total {success_count} tickets renamed.")
        self.run_btn.configure(state="normal", text="🚀 START BULK RENAMING")
        messagebox.showinfo("Success", f"Renaming Complete!\nProcessed {success_count} files.")
        try: os.startfile(folder)
        except: pass

if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk(); root.withdraw()
        messagebox.showerror("Error", "Launch via GOLEVENTS Hub.")
        sys.exit()