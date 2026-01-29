import sys
from pypdf import PdfReader

try:
    reader = PdfReader(r"C:\Users\kawka\Downloads\jalkshbdkljashlfhiosad893729814.pdf")
    for page in reader.pages:
        print(page.extract_text())
except Exception as e:
    print(e)
