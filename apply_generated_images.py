import os
import shutil

# Artifacts Dir (where generated images are)
ARTIFACTS_DIR = r"c:/Users/kawka/.gemini/antigravity/brain/e3e1a8ee-07a3-451f-8621-44b79322ba91"
# Guides Dir
GUIDES_DIR = r"c:/Users/kawka/.gemini/antigravity/scratch/app_cpp_conversion/assets/guides"

# Map: Tool Name -> Generated Image Filename Prefix
# Note: Filenames have timestamps, so we'll matching by prefix
map_tool_to_img = {
    "Check_listing": "check_listing_step1",
    "verify_orders": "verify_orders_step1",
    "check_price": "check_price_step1",
    "Calcul_stock": "calcul_stock_step1",
    "stock_report": "stock_report_step1",
    "pdfs_to_txt": "pdfs_to_txt_step1",
    "splitter_renamer": "splitter_renamer_step1",
    "renamer_fv": "renamer_fv_step1",
    "expander_seats": "expander_seats_step1",
    "qr_generator": "qr_generator_step1",
    "daily_report": "daily_report_step1",
    "placeholder": "placeholder_tool_step1"
}

def find_latest_image(prefix):
    files = [f for f in os.listdir(ARTIFACTS_DIR) if f.startswith(prefix) and f.endswith(".png")]
    if not files: return None
    # Sort by name (timestamp) descending
    files.sort(reverse=True)
    return os.path.join(ARTIFACTS_DIR, files[0])

def apply_images():
    print("Applying generated images...")
    for tool_name, prefix in map_tool_to_img.items():
        src_path = find_latest_image(prefix)
        if not src_path:
            print(f"Checking {prefix}: No image found.")
            continue
            
        dest_dir = os.path.join(GUIDES_DIR, tool_name)
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
            
        # We replace 1_img.png. If there are others like 2_img.png, we leave them or copy same?
        # Let's copy to 1_img.png for now as the "Step 1" visual.
        dest_path = os.path.join(dest_dir, "1_img.png")
        
        try:
            shutil.copy(src_path, dest_path)
            print(f"Updated {tool_name}: Copied to {dest_path}")
            
            # Optional: Copy to 2_img.png as well if it exists, to remove placeholder look entirely
            dest_path_2 = os.path.join(dest_dir, "2_img.png")
            if os.path.exists(dest_path_2):
                shutil.copy(src_path, dest_path_2)
                
        except Exception as e:
            print(f"Error copying {tool_name}: {e}")

if __name__ == "__main__":
    apply_images()
