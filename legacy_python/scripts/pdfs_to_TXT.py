import os
import re
import customtkinter as ctk
import requests
import subprocess
import sys
from tkinter import messagebox, filedialog

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
        PDFNameCleanerApp(self)

# --- NATURAL SORT LOGIC ---
def natural_sort(key):
    try:
        p1, p2, p3 = key.split('-')
        if p1.isdigit():
            p1 = (0, int(p1))
        else:
            p1 = (1, p1)
        return (p1, int(p2), int(p3))
    except:
        return ((9, ""), 9999, 9999)

# --- GUI INTERFACE ---
class PDFNameCleanerApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - PDF NAME CLEANER PRO 📄")
        self.master.geometry("800x700")
        self.master.configure(fg_color=BG_COLOR)

        # Header Section
        header_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        header_frame.pack(fill="x", padx=40, pady=(30, 20))
        
        ctk.CTkLabel(header_frame, text="📄 PDF NAME CLEANER", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header_frame, text="Extract 'Sector-Row-Seat' only from filenames into a TXT list.", font=("Arial", 13), text_color="gray60").pack(anchor="w")

        # Action Frame
        action_frame = ctk.CTkFrame(self.master, fg_color=CARD_BG, corner_radius=15, border_width=1, border_color=CARD_BORDER)
        action_frame.pack(fill="x", padx=40, pady=10)

        self.path_var = ctk.StringVar(value="No folder selected...")
        ctk.CTkEntry(action_frame, textvariable=self.path_var, font=("Arial", 12), width=500, height=45, 
                      fg_color="#0d0d0d", state="readonly").pack(side="left", padx=20, pady=20)
        
        ctk.CTkButton(action_frame, text="BROWSE", width=120, height=45, fg_color=ACCENT_COLOR, 
                      font=("Arial", 13, "bold"), command=self.browse_folder).pack(side="left", padx=(0, 20))

        # Process Button
        self.process_btn = ctk.CTkButton(self.master, text="🚀 START EXTRACTION & SAVE", height=55, 
                                          fg_color="#10b981", hover_color="#059669", text_color="black",
                                          font=("Arial", 16, "bold"), command=self.run_cleaner)
        self.process_btn.pack(pady=20, padx=40, fill="x")

        # Log Area
        self.log_area = ctk.CTkTextbox(self.master, font=("Consolas", 12), fg_color="#050505", 
                                        text_color="#00FF94", border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(fill="both", expand=True, padx=40, pady=(0, 40))

    def browse_folder(self):
        p = filedialog.askdirectory()
        if p:
            self.path_var.set(p)

    def log(self, msg, clear=False):
        self.log_area.configure(state="normal")
        if clear: self.log_area.delete("1.0", "end")
        self.log_area.insert("end", f"> {msg}\n")
        self.log_area.see("end")
        self.log_area.configure(state="disabled")

    def run_cleaner(self):
        folder_path = self.path_var.get()
        if folder_path == "No folder selected..." or not os.path.isdir(folder_path):
            messagebox.showwarning("Warning", "Please select a valid PDF folder first.")
            return

        self.log("Initializing process...", clear=True)
        clean_names = []

        # Logic Engine
        for root, dirs, files in os.walk(folder_path):
            for filename in files:
                if filename.lower().endswith(".pdf"):
                    name = filename.rsplit('.', 1)[0]
                    # pattern: sector-row-seat
                    match = re.match(r'^([A-Za-z0-9]+-\d+-\d+)', name)
                    if match:
                        clean_names.append(match.group(1))

        if not clean_names:
            self.log("❌ ERROR: No matching PDF names found (pattern: S-R-S).")
            return

        # Natural sort
        clean_names.sort(key=natural_sort)

        # Save TXT on Desktop
        desktop = os.path.join(os.path.expanduser("~"), "Desktop")
        output_file = os.path.join(desktop, "Clean_PDF_List.txt")

        try:
            with open(output_file, "w", encoding="utf-8") as f:
                for name in clean_names:
                    f.write(name + "\n")
            
            self.log(f"SUCCESS: {len(clean_names)} PDFs processed.")
            self.log(f"Saved to Desktop: Clean_PDF_List.txt")
            
            # Preview first 10
            self.log("\nPreview (First 10):")
            for i, name in enumerate(clean_names[:10]):
                self.log(f"   {i+1:2d}. {name}")
            
            messagebox.showinfo("Done", f"List generated successfully!\nTotal: {len(clean_names)}\nCheck your Desktop.")
            
            # Auto-open
            os.startfile(output_file)
        except Exception as e:
            self.log(f"❌ ERROR: Could not save file. {e}")

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk()
        root.withdraw()
        messagebox.showerror("Unauthorized", "Please launch via GOLEVENTS Hub.")
        sys.exit()