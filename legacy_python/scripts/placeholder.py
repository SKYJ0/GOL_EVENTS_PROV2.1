import os
import customtkinter as ctk
import requests
import subprocess
import sys
from tkinter import messagebox, filedialog
from reportlab.lib.pagesizes import A4
from reportlab.pdfgen import canvas

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
                messagebox.showerror("Security Alert", "Unauthorized Access!")
                sys.exit()
        except:
            sys.exit()

    def launch_main_tool(self):
        self.splash.destroy()
        self.deiconify()
        PlaceholderGeneratorApp(self)

# --- GUI INTERFACE ---
class PlaceholderGeneratorApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - PDF PLACEHOLDER PRO 🛠️")
        self.master.geometry("850x750")
        self.master.configure(fg_color=BG_COLOR)

        # Header Section
        header_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        header_frame.pack(fill="x", padx=40, pady=(30, 20))
        
        ctk.CTkLabel(header_frame, text="🛠️ PDF PLACEHOLDER PRO", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header_frame, text="Generate empty PDFs with custom names for your inventory.", font=("Arial", 13), text_color="gray60").pack(anchor="w")

        # Input Area
        ctk.CTkLabel(self.master, text="Paste names list (one per line):", font=("Arial", 12, "bold"), text_color="gray75").pack(anchor="w", padx=40)
        self.input_area = ctk.CTkTextbox(self.master, font=("Consolas", 13), fg_color=CARD_BG, 
                                          border_color=CARD_BORDER, border_width=1, corner_radius=15)
        self.input_area.pack(pady=10, padx=40, fill="both", expand=True)

        # Action Buttons
        btn_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        btn_frame.pack(fill="x", padx=40, pady=20)

        self.gen_btn = ctk.CTkButton(btn_frame, text="🚀 CHOOSE FOLDER & GENERATE", height=55, 
                                     fg_color="#10b981", hover_color="#059669", text_color="black",
                                     font=("Arial", 16, "bold"), command=self.process_generation)
        self.gen_btn.pack(side="left", fill="x", expand=True, padx=(0, 10))

        self.clear_btn = ctk.CTkButton(btn_frame, text="🗑️ CLEAR", height=55, width=120,
                                       fg_color="#e74c3c", hover_color="#c0392b", 
                                       command=lambda: self.input_area.delete("1.0", "end"))
        self.clear_btn.pack(side="left")

        # Log Area
        self.log_area = ctk.CTkTextbox(self.master, height=120, fg_color="#050505", text_color="#00FF88", 
                                        font=("Consolas", 12), state="disabled", border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(pady=(0, 30), padx=40, fill="x")

    def log(self, msg, clear=False):
        self.log_area.configure(state="normal")
        if clear: self.log_area.delete("1.0", "end")
        self.log_area.insert("end", f"> {msg}\n")
        self.log_area.see("end")
        self.log_area.configure(state="disabled")
        self.master.update() # Force update UI

    def process_generation(self):
        raw_text = self.input_area.get("1.0", "end").strip()
        if not raw_text:
            messagebox.showwarning("Empty", "Please paste some names first!")
            return

        # --- التصحيح: جعل نافذة الاختيار فوق البرنامج ---
        self.master.attributes("-topmost", False) 
        output_folder = filedialog.askdirectory(title="Where to save these PDFs?", parent=self.master)
        self.master.attributes("-topmost", True)

        if not output_folder:
            self.log("Operation cancelled by user.")
            return

        names = [line.strip() for line in raw_text.split("\n") if line.strip()]
        self.log(f"Starting process in: {output_folder}", clear=True)
        
        success_count = 0
        for name in names:
            try:
                # Sanitize name
                safe_name = "".join(c for c in name if c.isalnum() or c in ('-', '_')).strip()
                if not safe_name: 
                    self.log(f"Skipped invalid name: {name}")
                    continue
                
                file_path = os.path.join(output_folder, f"{safe_name}.pdf")
                
                # Logic PDF
                c = canvas.Canvas(file_path, pagesize=A4)
                c.setFont("Helvetica-Bold", 30)
                c.setFillGray(0.8)
                c.drawCentredString(A4[0]/2, A4[1]/2, "PLACEHOLDER TICKET")
                c.setFont("Helvetica", 12)
                c.drawCentredString(A4[0]/2, A4[1]/2 - 40, f"ID: {safe_name}")
                c.showPage()
                c.save()
                
                success_count += 1
                self.log(f"SUCCESS: {safe_name}.pdf")
            except Exception as e:
                self.log(f"FAILED: {name} | Error: {e}")

        self.log(f"\n✅ COMPLETE! Created: {success_count}/{len(names)}")
        messagebox.showinfo("Success", f"Process Finished!\n{success_count} PDFs created.")
        
        try:
            os.startfile(output_folder)
        except: pass

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk(); root.withdraw()
        messagebox.showerror("Error", "Launch via GOLEVENTS Hub.")
        sys.exit()