# ================== [ 1. BOOTSTRAP INTERNAL RUNNER ] ==================
import sys
import os
import runpy
import importlib

# This block allows the exe to act as a runner for external scripts (both .py and compiled .pyd)
if len(sys.argv) > 1:
    target = sys.argv[1]
    
    # Check if we are running a script/module
    # Heuristic: ends with .py or .pyd, or is a plain name in our scripts folder
    
    base_dir = os.path.dirname(os.path.abspath(sys.executable if getattr(sys, 'frozen', False) else __file__))
    scripts_dir = os.path.join(base_dir, "scripts")
    sys.path.append(base_dir)
    sys.path.append(scripts_dir)
    
    # Handle arguments
    script_arg = target
    key_arg = sys.argv[2] if len(sys.argv) > 2 else ""
    sys.argv = [script_arg, key_arg]

    if target.endswith(".py") and os.path.exists(target):
        # Run standard python script
        try: runpy.run_path(target, run_name="__main__")
        except Exception as e: print(f"Error running script: {e}")
        sys.exit(0)
        
    elif not target.startswith("-"):
        # Assume it's a module name (compiled or source)
        try:
            module_name = os.path.splitext(os.path.basename(target))[0]
            # Import the module
            mod = importlib.import_module(module_name)
            
            # If the build script wrapped the logic in def main():, call it.
            if hasattr(mod, "main"):
                mod.main()
            else:
                # Fallback for uncompiled .py files that still use if __name__ == "__main__":
                # We can try runpy only if it's NOT a pyd? 
                # Or just assume all pyds have main() now.
                # If it's a source file, importlib doesn't run main block.
                # But we are targeting the secured/compiled use case.
                pass
                
        except Exception as e:
            # vital: Show error to user since we have no console
            try:
                import tkinter.messagebox
                root = tkinter.Tk()
                root.withdraw()
                tkinter.messagebox.showerror("Script Error", f"Failed to run module '{target}':\n{e}")
                root.destroy()
            except:
                pass
        sys.exit(0)

# ================== [ 2. MAIN HUB CODE ] ==================
import customtkinter as ctk
import requests
import subprocess
from tkinter import messagebox, filedialog
from PIL import Image
import socket
import webbrowser
from datetime import datetime

VERSION = "v1.9.0" 
ACCENT_COLOR = "#1f6aa5"
PRO_COLOR = "#ffcc00"
BG_COLOR = "#0b0b0b"
CARD_BG = "#161616"
CARD_BORDER = "#252525"
LOCKED_BG = "#2a2a2a"
SUCCESS_COLOR = "#10b981"
ERROR_COLOR = "#f43f5e"

FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
UPDATE_JSON_URL = "https://raw.githubusercontent.com/SKYJ0/GOLEVENTS_PRO/main/update.json"

ctk.set_appearance_mode("Dark")

def resource_path(relative_path):
    try: base_path = sys._MEIPASS
    except Exception: base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, relative_path)

def get_public_ip():
    for url in ['https://api.ipify.org', 'https://ifconfig.me/ip']:
        try:
            r = requests.get(url, timeout=3)
            if r.status_code == 200: return r.text.strip()
        except: continue
    return "Unknown"

def get_hwid():
    try:
        cmd = 'wmic baseboard get serialnumber'
        return str(subprocess.check_output(cmd, shell=True), 'utf-8').split('\n')[1].strip()
    except: return socket.gethostname()

class GOLHubPro(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title(f"GOLEVENTS PRO {VERSION}")
        self.geometry("1300x850")
        self.configure(fg_color=BG_COLOR)
        
        self.current_role = "user"
        self.current_user_name = "GUEST"
        self.current_credits = 0
        self.license_key = ""
        self.home_frame = None
        self.tools_frame = None
        self.orders_frame = None

        self.current_tuto_index = 0
        self.current_tuto_steps = []

        self.grid_columnconfigure(1, weight=1); self.grid_rowconfigure(0, weight=1)
        self.navigation_frame = self.setup_sidebar()
        self.main_view = ctk.CTkFrame(self, fg_color="transparent")
        self.main_view.grid(row=0, column=1, sticky="nsew", padx=30, pady=10)
        
        self.login_frame = self.setup_login_ui()
        self.show_splash()

    # ---------------- [ COMPREHENSIVE TUTORIALS DATA ] ----------------
    def open_tutorial(self, tool_name, title):
        all_tutorials = {
            "Calcul_stock.py": [
                {
                    "text": "STEP 1: Click the 'BROWSE EVENT' button to open the folder selector. Navigate to your database and select the specific event folder you wish to analyze (Example: 2026.01.07 Bologna vs Atalanta). \n\nNote: Ensure the folder contains your ticket PDFs organized in sub-folders. \n\n\n\n STEP 2: Once you have highlighted the correct match folder, click the 'Select Folder' button at the bottom right of the window to load the path into the GOLEVENTS engine.", 
                    "img": "calc_step1.png"
                },
                {
                    "text": "STEP 3: Back in the main dashboard, simply click the large blue 'GENERATE & SAVE REPORT' button. The system will instantly begin scanning all sub-directories, extracting sector data, row numbers, and seat quantities. \n\n\n\n STEP 4: The live activity log will display a detailed breakdown of your current stock. A clean text report (.txt) will be automatically saved to a folder named 'Stock Files' on your Desktop for easy management.", 
                    "img": "calc_step2.png"
                },
                {
                    "text": "💡 PRO TIP - Odd-Even Mode: Check this box if you are processing tickets for specific stadiums (like Fiorentina) where seat numbering follows an odd/even logic.", 
                    "img": "calc_step3.png"
                }
            ],
            "check_price.py": [
                {
                    "text": "STEP 1: Visit the marketplace page in your browser. Right-click and select 'Save Page As...' to save it as an HTML file (Complete). \n\n\n\n STEP 2: Upload the saved HTML file into this tool. The engine will parse all competitor listings and highlight price differences compared to your current stock.", 
                    "img": "price_step1.png"
                }
            ],
            "expander_seats.py": [
                {
                    "text": "STEP 1: Copy your seat ranges from your source file (e.g., 'From 10S to 25S' or 'Row 5 Seats 1-10'). \n\n\n\n STEP 2: Paste the data into the input area. Click 'EXPAND' to generate a line-by-line list of every individual seat ID, ready for bulk uploading or mapping.", 
                    "img": "exp_step1.png"
                }
            ],
            "pdfs_to_txt.py": [
                {
                    "text": "STEP 1: Select the folder containing your finalized PDF tickets. Sub-folders are supported. \n\n\n\n STEP 2: The tool will extract all filenames (Formatted as Sector-Row-Seat) and export them into a clean TXT file on your Desktop for final verification.", 
                    "img": "p2t_step1.png"
                }
            ],
            "placeholder.py": [
                {
                    "text": "STEP 1: Paste a list of names or Ticket IDs that you are currently missing from your inventory. \n\n\n\n STEP 2: Choose an output folder. The tool will generate empty 'Placeholder' PDFs with these names to keep your directory structure consistent.", 
                    "img": "hold_step1.png"
                }
            ],
            "qr_generator.py": [
                {
                    "text": "STEP 1: Paste your alphanumeric symbols, ticket codes, or URLs into the text box (one per line). \n\n\n\n STEP 2: Click 'Choose Folder' to generate high-resolution 1035x1035 QR codes. The images are saved without borders for professional printing.", 
                    "img": "qr_step1.png"
                }
            ],
            "Check_listing.py": [
                {
                    "text": "STEP 1: Save your personal active listings dashboard from the marketplace as an HTML file. \n\n\n\n STEP 2: Load the file here to verify your market positioning. The report will tell you if you are 'FIRST' or if other sellers have undercut your price.", 
                    "img": "list_step1.png"
                }
            ],
            "verify_orders.py": [
                {
                    "text": "STEP 1: Upload your Sales CSV or XLSX file and select your local match folder. \n\n\n\n STEP 2: Run the scan. The system will perform a cross-check to find any missing ticket IDs or duplicated orders in your folder structure.", 
                    "img": "verify_step1.png"
                }
            ],
            "renamer_fv.py": [
                {
                    "text": "STEP 1: Select the folder with raw PDF tickets from your suppliers. \n\n\n\n STEP 2: The tool uses OCR to read the 'Face Value' inside the PDF. It will automatically rename files to include the price (e.g., Ticket-FV50p00.pdf).", 
                    "img": "ren_step1.png"
                }
            ],
            "splitter_renamer.py": [
                {
                    "text": "STEP 1: Point to an 'Input' folder containing multi-page bulk PDFs and choose an 'Output' folder. \n\n\n\n STEP 2: The engine will split every page and rename it using the unique Seat Code found on the ticket. Optimized for AC Milan & Vivaticket.", 
                    "img": "split_step1.png"
                }
            ]
        }

        self.current_tuto_steps = all_tutorials.get(tool_name, [{"text": "Tutorial guide coming soon.", "img": "none.png"}])
        self.current_tuto_index = 0
        self.tuto_win = ctk.CTkToplevel(self)
        self.tuto_win.title(f"Guide: {title}"); self.tuto_win.geometry("800x750"); self.tuto_win.configure(fg_color="#0f0f0f")
        self.tuto_win.attributes("-topmost", True); self.tuto_win.grab_set()

        self.tuto_header = ctk.CTkLabel(self.tuto_win, text="", font=("Arial", 20, "bold"), text_color=ACCENT_COLOR); self.tuto_header.pack(pady=20)
        self.tuto_img_label = ctk.CTkLabel(self.tuto_win, text=""); self.tuto_img_label.pack(pady=10)
        self.tuto_text = ctk.CTkLabel(self.tuto_win, text="", font=("Arial", 14), wraplength=700, justify="left"); self.tuto_text.pack(pady=20, padx=50)

        ctrl = ctk.CTkFrame(self.tuto_win, fg_color="transparent"); ctrl.pack(side="bottom", fill="x", pady=30, padx=60)
        self.prev_btn = ctk.CTkButton(ctrl, text="⟵ BACK", width=100, fg_color="#333", command=self.prev_step); self.prev_btn.pack(side="left")
        self.next_btn = ctk.CTkButton(ctrl, text="NEXT ⟶", width=100, fg_color=ACCENT_COLOR, command=self.next_step); self.next_btn.pack(side="right")
        self.update_tuto_ui()

    def update_tuto_ui(self):
        step = self.current_tuto_steps[self.current_tuto_index]
        self.tuto_header.configure(text=f"Step {self.current_tuto_index + 1} of {len(self.current_tuto_steps)}")
        self.tuto_text.configure(text=step["text"])
        try:
            img = ctk.CTkImage(Image.open(resource_path(os.path.join("tutorials", step["img"]))), size=(650, 350))
            self.tuto_img_label.configure(image=img, text="")
        except: self.tuto_img_label.configure(image=None, text="[ Image Asset Missing ]")
        self.prev_btn.configure(state="normal" if self.current_tuto_index > 0 else "disabled")
        self.next_btn.configure(text="FINISH ✅" if self.current_tuto_index == len(self.current_tuto_steps)-1 else "NEXT ⟶")

    def next_step(self):
        if self.current_tuto_index < len(self.current_tuto_steps)-1: self.current_tuto_index += 1; self.update_tuto_ui()
        else: self.tuto_win.destroy()
    def prev_step(self):
        if self.current_tuto_index > 0: self.current_tuto_index -= 1; self.update_tuto_ui()

    # ---------------- [ MAIN APP FUNCTIONS ] ----------------
    def show_splash(self):
        self.splash_frame = ctk.CTkFrame(self, fg_color=BG_COLOR); self.splash_frame.place(relx=0, rely=0, relwidth=1, relheight=1)
        try:
            knight_img = ctk.CTkImage(Image.open(resource_path("knight.png")), size=(350, 350))
            ctk.CTkLabel(self.splash_frame, image=knight_img, text="").pack(pady=(150, 20))
        except: pass
        self.splash_msg = ctk.CTkLabel(self.splash_frame, text="LOADING GOLEVENTS SECURE ENGINE...", font=("Arial", 14), text_color=ACCENT_COLOR); self.splash_msg.pack()
        self.load_bar = ctk.CTkProgressBar(self.splash_frame, width=400, progress_color=ACCENT_COLOR); self.load_bar.set(0); self.load_bar.pack(pady=20)
        self.after(500, lambda: self.animate_loading(0))

    def animate_loading(self, val):
        if val <= 1.0: self.load_bar.set(val); self.after(30, lambda: self.animate_loading(val + 0.02))
        else: self.splash_frame.destroy(); self.show_frame(self.login_frame)

    def setup_sidebar(self):
        frame = ctk.CTkFrame(self, fg_color="#0d0d0d", width=260); frame.grid_propagate(False)
        ctk.CTkLabel(frame, text="GOLEVENTS 👑", font=("Arial", 28, "bold"), text_color=ACCENT_COLOR).pack(pady=(50, 60))
        self.home_btn = self.create_nav_item(frame, "Dashboard", "🏠", self.show_home)
        self.tools_btn = self.create_nav_item(frame, "Inventory Tools", "📦", self.show_tools)
        self.orders_btn = self.create_nav_item(frame, "Orders & Verify", "📜", self.show_orders)
        ctk.CTkLabel(frame, text="Created by Omar", font=("Arial", 12), text_color="gray30").pack(side="bottom", pady=25)
        return frame

    def create_nav_item(self, parent, text, icon, command):
        btn = ctk.CTkButton(parent, text=f"  {icon}   {text}", height=55, fg_color="transparent", hover_color="#1a1a1a", anchor="w", font=("Arial", 15, "bold"), corner_radius=12, command=command)
        btn.pack(fill="x", padx=25, pady=6); return btn

    def setup_home_ui(self):
        f = ctk.CTkFrame(self.main_view, fg_color="transparent")
        ctk.CTkLabel(f, text="GOLEVENTS PREMIUM HUB", font=("Arial", 32, "bold"), text_color=ACCENT_COLOR).pack(anchor="w", pady=(20, 5))
        stats = ctk.CTkFrame(f, fg_color="transparent"); stats.pack(fill="x", pady=15)
        self.create_stat_card(stats, "Member Name", self.current_user_name.upper(), "white", "👤")
        role_disp = "PRO MEMBER" if self.current_role == "pro" else "BASIC USER"
        self.create_stat_card(stats, "Account Type", role_disp, PRO_COLOR if self.current_role == "pro" else "#3B8ED0", "👑")
        
        # COIN COLOR UPDATED TO YELLOW (PRO_COLOR)
        if self.current_role == "pro" or self.current_credits > 0:
            self.create_stat_card(stats, "Balance Status", f"🪙 {self.current_credits} COINS", PRO_COLOR, "💰")

        guide = ctk.CTkFrame(f, fg_color="#121212", corner_radius=15, border_width=1, border_color="#1f1f1f"); guide.pack(fill="x", pady=20)
        ctk.CTkLabel(guide, text="📌 SYSTEM ANNOUNCEMENTS", font=("Arial", 18, "bold")).pack(anchor="w", padx=25, pady=15)
        notices = [f"• Welcome {self.current_user_name}!", f"• System Rank: {role_disp.upper()}.", "• 1 Coin = 1 Day of PRO usage.", "• For coin top-ups, contact Omar."]
        for i in notices: ctk.CTkLabel(guide, text=i, font=("Arial", 14), text_color="gray75").pack(anchor="w", padx=40, pady=3)
        self.log_box = ctk.CTkTextbox(f, height=120, fg_color="#0a0a0a", text_color="#00FF88", font=("Consolas", 12), border_width=1, border_color="#1f1f1f")
        self.log_box.pack(fill="x", side="bottom", pady=10)
        return f

    def create_stat_card(self, parent, title, value, color, icon):
        card = ctk.CTkFrame(parent, fg_color=CARD_BG, corner_radius=15, border_width=1, border_color=CARD_BORDER, height=140)
        card.pack(side="left", padx=10, fill="both", expand=True); card.pack_propagate(False)
        ctk.CTkLabel(card, text=f"{icon} {title}", font=("Arial", 12), text_color="gray70").pack(pady=(20, 0), padx=25, anchor="w")
        ctk.CTkLabel(card, text=value, font=("Arial", 18, "bold"), text_color=color).pack(pady=(5, 0), padx=25, anchor="w")

    def setup_tools_ui(self):
        f = ctk.CTkFrame(self.main_view, fg_color="transparent")
        ctk.CTkLabel(f, text="INVENTORY TOOLS", font=("Arial", 30, "bold")).pack(anchor="w", pady=(20, 10))
        container = ctk.CTkScrollableFrame(f, fg_color="transparent", height=600); container.pack(fill="both", expand=True)
        self.create_tool_card(container, "Stock Calc", "Inventory stock calculator.", "Calcul_stock.py", 0, 0)
        self.create_tool_card(container, "Price Check", "Monitor market prices.", "check_price.py", 0, 1, is_pro=True)
        self.create_tool_card(container, "Seat Expander", "Seating map generator.", "expander_seats.py", 0, 2)
        self.create_tool_card(container, "PDF to TXT", "Extract PDF data to text.", "pdfs_to_txt.py", 1, 0)
        self.create_tool_card(container, "Placeholder", "Temporary PDF generator.", "placeholder.py", 1, 1)
        self.create_tool_card(container, "QR Generator", "Batch QR Codes.", "qr_generator.py", 1, 2)
        return f

    def setup_orders_ui(self):
        f = ctk.CTkFrame(self.main_view, fg_color="transparent")
        ctk.CTkLabel(f, text="ORDERS & VERIFICATION", font=("Arial", 30, "bold")).pack(anchor="w", pady=(20, 10))
        container = ctk.CTkScrollableFrame(f, fg_color="transparent", height=600); container.pack(fill="both", expand=True)
        self.create_tool_card(container, "Listing Checker", "Verify listings status.", "Check_listing.py", 0, 0, is_pro=True)
        self.create_tool_card(container, "Order Verifier", "Ensure orders accuracy.", "verify_orders.py", 0, 1, is_pro=True)
        self.create_tool_card(container, "Renamer + FV", "Rename files with FV.", "renamer_fv.py", 0, 2)
        self.create_tool_card(container, "Splitter+Renamer", "PDF split and rename.", "splitter_renamer.py", 1, 0)
        return f

    def create_tool_card(self, parent, title, desc, script_name, row, col, is_pro=False):
        card = ctk.CTkFrame(parent, fg_color=CARD_BG, corner_radius=18, border_width=1, border_color=CARD_BORDER, height=270, width=340)
        card.grid(row=row, column=col, padx=15, pady=15, sticky="nsew"); card.grid_propagate(False)
        
        ctk.CTkButton(card, text="📖 Guide", font=("Arial", 11, "bold"), fg_color="#333", width=80, height=24, corner_radius=6, command=lambda: self.open_tutorial(script_name, title)).place(x=20, y=15)
        if is_pro: ctk.CTkLabel(card, text="PRO", fg_color=PRO_COLOR, text_color="black", font=("Arial", 11, "bold"), corner_radius=8, width=58, height=26).place(relx=1.0, x=-12, y=12, anchor="ne")
        ctk.CTkLabel(card, text=title, font=("Arial", 22, "bold")).pack(anchor="w", padx=25, pady=(55, 5))
        ctk.CTkLabel(card, text=desc, font=("Arial", 13), text_color="gray60", wraplength=280, justify="left").pack(anchor="w", padx=25)
        
        is_locked = is_pro and self.current_role != "pro"
        btn_txt, btn_clr = ("🔒  LOCKED", LOCKED_BG) if is_locked else ("LAUNCH TOOL", ACCENT_COLOR)
        ctk.CTkButton(card, text=btn_txt, fg_color=btn_clr, height=46, corner_radius=12, command=lambda: self.run_script(script_name) if not is_locked else messagebox.showwarning("PRO", "Requires PRO Account.")).pack(side="bottom", fill="x", padx=20, pady=25)

    def run_script(self, name):
        # Determine behavior based on whether we are frozen (packaged) or dev
        if getattr(sys, 'frozen', False):
            py_exe = sys.executable
            base_dir = os.path.dirname(sys.executable)
        else:
            py_exe = sys.executable
            base_dir = os.path.dirname(os.path.abspath(__file__))

        script_base_name = os.path.splitext(name)[0]
        
        # Priority 1: Check for External Compiled Script (.pyd)
        # We pass the module name to the runner
        ext_pyd = os.path.join(base_dir, "scripts", script_base_name + ".pyd")
        if os.path.exists(ext_pyd):
            self.execute_runner(py_exe, script_base_name, base_dir)
            return

        # Priority 2: Check for External Python Script (.py)
        # We pass the full path
        ext_py = os.path.join(base_dir, "scripts", name)
        if os.path.exists(ext_py):
            self.execute_runner(py_exe, ext_py, base_dir)
            return
            
        # Priority 3: Internal Resources (bundled)
        # Could be .py source or compiled module bundled
        # If bundled as module, we can try importing by name
        # But we need to know if it exists. 
        # For simplicity, if standard files missing, try running by name (fallback to internal module import)
        
        # Let's try to find internal .py just in case
        int_py = resource_path(os.path.join("scripts", name))
        if int_py and os.path.exists(int_py):
            self.execute_runner(py_exe, int_py, base_dir)
            return

        # Fallback: Try running as module name (for internal compiled modules)
        self.execute_runner(py_exe, script_base_name, base_dir)

    def execute_runner(self, exe, target, base_dir):
        try:
            env = os.environ.copy()
            # Ensure scripts dir is in PYTHONPATH
            scripts_dir = os.path.join(base_dir, "scripts")
            env["PYTHONPATH"] = scripts_dir + os.path.pathsep + base_dir + os.path.pathsep + env.get("PYTHONPATH", "")
            
            cmd = [exe]
            # If dev mode (not frozen) and running a module (not .py file), invoke main.py as runner
            if not getattr(sys, 'frozen', False) and not target.lower().endswith(".py"):
                cmd.append(os.path.abspath(__file__))
            
            cmd.append(target)
            cmd.append(self.license_key)
            
            # Launch the script using the determined interpreter
            # creationflags=0x08000000 prevents a console window from popping up
            subprocess.Popen(cmd, env=env, creationflags=0x08000000)
        except Exception as e:
            messagebox.showerror("Execution Error", f"Failed to launch script: {e}")

    def process_auth(self):
        key = self.key_input.get().strip()
        if not key: return
        hwid = get_hwid(); ip = get_public_ip(); device = socket.gethostname()
        try:
            res = requests.get(f"{FIREBASE_URL}/keys/{key}.json").json()
            if res and res.get("status") == "active":
                if res.get("hwid") and res.get("hwid") != hwid: messagebox.showerror("HWID", "Mismatch!"); return
                self.license_key = key; self.current_role = res.get("role", "user"); self.current_user_name = res.get("name", "GUEST")
                self.current_credits = res.get("credits", 0); last_check = res.get("last_check_date", ""); today = datetime.now().strftime("%Y-%m-%d")
                if self.current_role == "pro" and last_check != today:
                    if self.current_credits > 0:
                        self.current_credits -= 1
                        requests.patch(f"{FIREBASE_URL}/keys/{key}.json", json={"credits": self.current_credits, "last_check_date": today})
                    if self.current_credits <= 0:
                        self.current_role = "user"; self.current_credits = 0
                        requests.patch(f"{FIREBASE_URL}/keys/{key}.json", json={"role": "user", "credits": 0})
                requests.patch(f"{FIREBASE_URL}/keys/{key}.json", json={"last_ip": ip, "device_name": device, "hwid": hwid})
                self.home_frame = self.setup_home_ui(); self.tools_frame = self.setup_tools_ui(); self.orders_frame = self.setup_orders_ui()
                self.navigation_frame.grid(row=0, column=0, sticky="nsew"); self.show_home()
            else: self.msg_lbl.configure(text="Invalid Key")
        except: self.msg_lbl.configure(text="Connection Error")

    def show_frame(self, f):
        if hasattr(self, 'login_frame') and self.login_frame: self.login_frame.pack_forget()
        if hasattr(self, 'home_frame') and self.home_frame: self.home_frame.pack_forget()
        if hasattr(self, 'tools_frame') and self.tools_frame: self.tools_frame.pack_forget()
        if hasattr(self, 'orders_frame') and self.orders_frame: self.orders_frame.pack_forget()
        if f: f.pack(fill="both", expand=True)

    def show_home(self): self.show_frame(self.home_frame)
    def show_tools(self): self.show_frame(self.tools_frame)
    def show_orders(self): self.show_frame(self.orders_frame)
    def write_log(self, t):
        try: self.log_box.insert("end", f"[INFO] {t}\n"); self.log_box.see("end")
        except: pass
    
    def setup_login_ui(self):
        f = ctk.CTkFrame(self.main_view, fg_color="transparent"); box = ctk.CTkFrame(f, fg_color="#121212", width=420, height=420, corner_radius=22, border_width=1, border_color="#222"); box.place(relx=0.5, rely=0.5, anchor="center")
        ctk.CTkLabel(box, text="GOLEVENTS ACCESS", font=("Arial", 28, "bold"), text_color=ACCENT_COLOR).pack(pady=(60, 45))
        self.key_input = ctk.CTkEntry(box, placeholder_text="License Key", width=320, height=55, justify="center")
        self.key_input.pack(); self.key_input.bind("<Return>", lambda event: self.process_auth())
        ctk.CTkButton(box, text="LOGIN", height=55, width=320, font=("Arial", 15, "bold"), command=self.process_auth).pack(pady=35)
        self.msg_lbl = ctk.CTkLabel(box, text="", text_color="red"); self.msg_lbl.pack(); return f

if __name__ == "__main__":
    app = GOLHubPro(); app.mainloop()