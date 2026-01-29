import sys
import subprocess

try:
    from PIL import Image, ImageTk
except ImportError:
    print("Pillow library not found. Installing automatically...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "pillow"])
        from PIL import Image, ImageTk
        print("Pillow installed successfully!")
    except Exception as e:
        print(f"Failed to install Pillow: {e}")
        print("Please run: pip install pillow")
        sys.exit(1)

import os
import shutil
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter import font as tkfont

# Configuration
ASSETS_DIR = r"c:/Users/kawka/.gemini/antigravity/scratch/app_cpp_conversion/assets/guides"

class GuideEditorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("GOL Guide Editor")
        self.root.geometry("1000x700")
        self.root.configure(bg="#0f172a")

        self.current_tool = None
        self.steps = []
        self.images = [] # Keep references to avoid GC

        self.setup_styles()
        self.setup_ui()
        self.load_tools()

    def setup_styles(self):
        style = ttk.Style()
        style.theme_use('clam')
        
        # Colors
        bg_dark = "#0f172a"
        bg_card = "#1e293b"
        accent = "#3b82f6"
        text_light = "#f1f5f9"
        
        style.configure("TFrame", background=bg_dark)
        style.configure("Card.TFrame", background=bg_card, borderwidth=1, relief="solid")
        style.configure("TLabel", background=bg_dark, foreground=text_light, font=("Segoe UI", 10))
        style.configure("Header.TLabel", font=("Segoe UI", 16, "bold"), foreground=accent)
        style.configure("TButton", background=accent, foreground="white", borderwidth=0, font=("Segoe UI", 9, "bold"))
        style.map("TButton", background=[('active', "#2563eb")])
        
        self.bg_card = bg_card
        self.text_light = text_light

    def setup_ui(self):
        # Main Layout: Sidebar (Tools) + Content (Steps)
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)

        # --- Sidebar ---
        sidebar = ttk.Frame(main_frame, width=250)
        sidebar.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 20))
        
        ttk.Label(sidebar, text="TOOLS", style="Header.TLabel").pack(anchor="w", pady=(0, 10))
        
        self.tool_list = tk.Listbox(sidebar, bg=self.bg_card, fg=self.text_light, 
                                    selectbackground="#3b82f6", selectforeground="white",
                                    borderwidth=0, highlightthickness=0, font=("Segoe UI", 11))
        self.tool_list.pack(fill=tk.BOTH, expand=True)
        self.tool_list.bind('<<ListboxSelect>>', self.on_tool_select)

        # --- Content Area ---
        content = ttk.Frame(main_frame)
        content.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # Toolbar
        toolbar = ttk.Frame(content)
        toolbar.pack(fill=tk.X, pady=(0, 10))
        
        self.lbl_current_tool = ttk.Label(toolbar, text="Select a Tool", style="Header.TLabel")
        self.lbl_current_tool.pack(side=tk.LEFT)

        btn_refresh = ttk.Button(toolbar, text="🔄 Reload", command=self.load_tools)
        btn_refresh.pack(side=tk.RIGHT, padx=5)
        
        btn_save = ttk.Button(toolbar, text="💾 Save Changes", command=self.save_current_tool)
        btn_save.pack(side=tk.RIGHT, padx=5)

        # Scrollable Steps Area
        self.canvas = tk.Canvas(content, bg="#0f172a", highlightthickness=0)
        self.scrollbar = ttk.Scrollbar(content, orient="vertical", command=self.canvas.yview)
        self.scroll_frame = ttk.Frame(self.canvas)

        self.scroll_frame.bind(
            "<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all"))
        )

        self.canvas.create_window((0, 0), window=self.scroll_frame, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scrollbar.set)

        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Mousewheel
        self.canvas.bind_all("<MouseWheel>", self._on_mousewheel)

    def _on_mousewheel(self, event):
        self.canvas.yview_scroll(int(-1*(event.delta/120)), "units")

    def load_tools(self):
        self.tool_list.delete(0, tk.END)
        if not os.path.exists(ASSETS_DIR):
            os.makedirs(ASSETS_DIR)
            
        tools = sorted([d for d in os.listdir(ASSETS_DIR) if os.path.isdir(os.path.join(ASSETS_DIR, d))])
        for tool in tools:
            self.tool_list.insert(tk.END, tool)

    def on_tool_select(self, event):
        selection = self.tool_list.curselection()
        if not selection:
            return
        
        tool_name = self.tool_list.get(selection[0])
        self.load_tool_content(tool_name)

    def load_tool_content(self, tool_name):
        self.current_tool = tool_name
        self.lbl_current_tool.config(text=f"Editing: {tool_name}")
        
        # Clear existing widgets
        for widget in self.scroll_frame.winfo_children():
            widget.destroy()
        
        tool_dir = os.path.join(ASSETS_DIR, tool_name)
        
        # Find steps
        # Expecting 1_txt.txt, 1_img.png etc
        files = os.listdir(tool_dir)
        step_nums = set()
        for f in files:
            if "_" in f:
                try:
                    num = int(f.split("_")[0])
                    step_nums.add(num)
                except:
                    pass
        
        sorted_steps = sorted(list(step_nums))
        self.steps_ui = []
        
        for num in sorted_steps:
            self.create_step_ui(num, tool_dir)

        # Add "New Step" button at bottom
        btn_add = ttk.Button(self.scroll_frame, text="+ ADD STEP", command=self.add_step)
        btn_add.pack(pady=20, ipadx=20, ipady=5)

    def create_step_ui(self, step_num, tool_dir):
        # Container
        card = tk.Frame(self.scroll_frame, bg=self.bg_card, bd=1, relief="solid")
        card.pack(fill=tk.X, pady=10, padx=5, ipady=10)
        
        # Step Header
        header = tk.Frame(card, bg=self.bg_card)
        header.pack(fill=tk.X, padx=15, pady=5)
        
        tk.Label(header, text=f"STEP {step_num}", bg=self.bg_card, fg="#3b82f6", font=("Segoe UI", 12, "bold")).pack(side=tk.LEFT)
        # Delete Button
        btn_del = tk.Button(header, text="✕", bg="#ef4444", fg="white", bd=0, 
                            command=lambda n=step_num: self.delete_step(n))
        btn_del.pack(side=tk.RIGHT)

        # Content Row
        row = tk.Frame(card, bg=self.bg_card)
        row.pack(fill=tk.X, padx=15, pady=5)

        # Left: Image
        img_frame = tk.Frame(row, bg=self.bg_card)
        img_frame.pack(side=tk.LEFT, padx=(0, 20))
        
        img_path = os.path.join(tool_dir, f"{step_num}_img.png")
        if not os.path.exists(img_path):
             # Try jpg or others? For now assume png
             pass

        # Thumbnail
        lbl_img = tk.Label(img_frame, bg="black", width=240, height=135) # approx 16:9 box
        lbl_img.pack()
        
        self.load_thumbnail(lbl_img, img_path)

        btn_img = ttk.Button(img_frame, text="Change Image", command=lambda l=lbl_img, p=img_path: self.change_image(l, p))
        btn_img.pack(fill=tk.X, pady=5)

        # Right: Text
        txt_frame = tk.Frame(row, bg=self.bg_card)
        txt_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        tk.Label(txt_frame, text="Description:", bg=self.bg_card, fg="gray").pack(anchor="w")
        
        txt_path = os.path.join(tool_dir, f"{step_num}_txt.txt")
        initial_text = ""
        if os.path.exists(txt_path):
            with open(txt_path, "r", encoding="utf-8") as f:
                initial_text = f.read()

        txt_box = tk.Text(txt_frame, height=5, bg="#0f172a", fg="white", insertbackground="white", bd=0, padx=5, pady=5, font=("Segoe UI", 10))
        txt_box.insert("1.0", initial_text)
        txt_box.pack(fill=tk.BOTH, expand=True)

        # Store references for saving
        self.steps_ui.append({
            "num": step_num,
            "txt_widget": txt_box,
            "txt_path": txt_path
            # Image is handled immediately on change
        })

    def load_thumbnail(self, label, path):
        try:
            pil_img = Image.open(path)
            pil_img.thumbnail((240, 135))
            tk_img = ImageTk.PhotoImage(pil_img)
            label.config(image=tk_img, width=240, height=135)
            label.image = tk_img # Keep reference
        except Exception as e:
            label.config(text="NO IMAGE\n(Install Pillow)", fg="white")

    def change_image(self, label, target_path):
        file_path = filedialog.askopenfilename(filetypes=[("Images", "*.png;*.jpg;*.jpeg;*.bmp")])
        if file_path:
            # Copy to target
            try:
                # Convert to PNG for consistency if needed, or just copy
                # We enforce PNG naming in this system
                img = Image.open(file_path)
                img.save(target_path, "PNG")
                self.load_thumbnail(label, target_path)
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save image: {str(e)}")

    def save_current_tool(self):
        if not self.current_tool: return
        
        try:
            for step in self.steps_ui:
                new_text = step["txt_widget"].get("1.0", tk.END).strip()
                with open(step["txt_path"], "w", encoding="utf-8") as f:
                    f.write(new_text)
            messagebox.showinfo("Success", "Guide updated successfully!")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save: {str(e)}")

    def add_step(self):
        if not self.current_tool: return
        
        # Calculate next number
        next_num = 1
        if self.steps_ui:
            next_num = max(s["num"] for s in self.steps_ui) + 1
            
        tool_dir = os.path.join(ASSETS_DIR, self.current_tool)
        
        # Create empty files
        img_path = os.path.join(tool_dir, f"{next_num}_img.png")
        txt_path = os.path.join(tool_dir, f"{next_num}_txt.txt")
        
        # Create text
        with open(txt_path, "w") as f: f.write("New step description...")
        
        # Create dummy image or copy last one
        placeholder = Image.new('RGB', (100, 100), color = '#1e293b')
        placeholder.save(img_path)

        # Refresh
        self.load_tool_content(self.current_tool)

    def delete_step(self, step_num):
        if not messagebox.askyesno("Confirm", f"Delete Step {step_num}?"):
            return
            
        tool_dir = os.path.join(ASSETS_DIR, self.current_tool)
        
        # Delete content
        for ext in ["_img.png", "_txt.txt"]:
            f = os.path.join(tool_dir, f"{step_num}{ext}")
            if os.path.exists(f): os.remove(f)
            
        # Reorder remaining? 
        # For simplicity, we just reload. Gaps might exist (1, 3). 
        # App logic should handle gaps or we should rename.
        # Let's simple rename to close gaps for cleanliness.
        self.reorder_steps(tool_dir)
        self.load_tool_content(self.current_tool)

    def reorder_steps(self, tool_dir):
        # Rename logic to 1..N
        files = os.listdir(tool_dir)
        pairs = []
        for f in files:
            if "_txt.txt" in f:
                num = int(f.split("_")[0])
                pairs.append(num)
        
        pairs.sort()
        
        for i, old_num in enumerate(pairs):
            new_num = i + 1
            if old_num != new_num:
                # Rename txt
                os.rename(os.path.join(tool_dir, f"{old_num}_txt.txt"), 
                          os.path.join(tool_dir, f"{new_num}_txt.txt"))
                # Rename img
                old_img = os.path.join(tool_dir, f"{old_num}_img.png")
                if os.path.exists(old_img):
                    os.rename(old_img, os.path.join(tool_dir, f"{new_num}_img.png"))

if __name__ == "__main__":
    # Check dependencies
    try:
        import PIL
    except ImportError:
        import subprocess
        print("Installing Pillow for image handling...")
        subprocess.check_call(["pip", "install", "pillow"])
        print("Restarting app...")
        import sys
        os.execv(sys.executable, ['python'] + sys.argv)

    root = tk.Tk()
    app = GuideEditorApp(root)
    root.mainloop()
