import os
import shutil

# Root assets directory
ASSETS_DIR = r"c:/Users/kawka/.gemini/antigravity/scratch/app_cpp_conversion/assets/guides"
PLACEHOLDER_IMG = r"c:/Users/kawka/.gemini/antigravity/brain/e3e1a8ee-07a3-451f-8621-44b79322ba91/guide_placeholder_1769127891832.png"

# Tool Data: Name -> Steps (List of Strings)
tools = {
    "Check_listing": [
        "Use the 'Browse' button to select your listing PDF file. The system supports standard listing formats.",
        "Click 'Check Listing' to verify the data against the current database. Results will appear in the log window below."
    ],
    "verify_orders": [
        "Upload your order confirmation PDF or folder containing multiple orders.",
        "The system will cross-reference orders with available stock and highlight any discrepancies in red."
    ],
    "check_price": [
        "Enter the event name or ID to fetch current market pricing.",
        "Review the price trends graph to make informed pricing decisions for your inventory."
    ],
    "Calcul_stock": [
        "Select the root folder containing your ticket PDFs. The system will recursively scan all subfolders.",
        "Click 'Calculate' to generate a real-time count of physical stock vs. pending orders."
    ],
    "stock_report": [
        "Ensure your stock calculation is up to date before running this report.",
        "Click 'Generate Report' to create a detailed breakdown of net stock by sector and category.",
        "The report is automatically saved to your Desktop."
    ],
    "pdfs_to_txt": [
        "Select a folder containing PDF files you wish to convert.",
        "Click 'Convert' to extract text content. Output text files will be saved in the same directory."
    ],
    "splitter_renamer": [
        "Choose a multi-page PDF files that needs to be split.",
        "Configure the naming pattern (e.g., '{OriginalName}_{PageNum}') and click 'Split' to process."
    ],
    "daily_report": [
        "This tool automates your daily workflow. Ensure all data sources are connected.",
        "Click 'Run Daily Cycle' to execute all daily checks and generate the summary report."
    ],
    "renamer_fv": [
        "Select the folder with files to rename.",
        "The tool will apply the 'FV' naming convention automatically based on file content detection."
    ],
    "expander_seats": [
        "Load a seating chart or list of seat numbers.",
        "The tool will expand ranges (e.g., '1-10') into individual seat entries for detailed tracking."
    ],
    "qr_generator": [
        "Input a list of codes or select a file containing data for QR generation.",
        "Click 'Generate' to create high-resolution QR codes for each entry. Files are saved to the output folder."
    ],
    "placeholder": [
        "This is a utility for generating temporary placeholder PDFs for testing.",
        "Specify the number of files and naming pattern, then click 'Generate'."
    ]
}

def create_guides():
    if not os.path.exists(ASSETS_DIR):
        print(f"Creating assets dir: {ASSETS_DIR}")
        os.makedirs(ASSETS_DIR)

    # Ensure placeholder exists in source
    if not os.path.exists(PLACEHOLDER_IMG):
        print("Warning: Placeholder image not found at source. Skipping image copy.")

    for tool_name, steps in tools.items():
        tool_dir = os.path.join(ASSETS_DIR, tool_name)
        if not os.path.exists(tool_dir):
            os.makedirs(tool_dir)
            print(f"Created dir: {tool_dir}")

        for i, step_desc in enumerate(steps):
            step_num = i + 1
            
            # Write Text
            txt_path = os.path.join(tool_dir, f"{step_num}_txt.txt")
            with open(txt_path, "w", encoding="utf-8") as f:
                f.write(step_desc)
            
            # Copy Image
            if os.path.exists(PLACEHOLDER_IMG):
                img_path = os.path.join(tool_dir, f"{step_num}_img.png")
                shutil.copy(PLACEHOLDER_IMG, img_path)
        
        print(f"Generated {len(steps)} steps for {tool_name}")

if __name__ == "__main__":
    create_guides()
