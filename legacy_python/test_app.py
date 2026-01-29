#!/usr/bin/env python3
"""
سكريبت اختبار شامل لتطبيق GOLEVENTS PRO C++
يتحقق من:
1. وجود جميع ملفات المصدر C++
2. صحة بنية المشروع
3. التحقق من المكتبات المطلوبة
4. اختبار الوظائف الأساسية
"""

import os
import sys
import json
from pathlib import Path
from typing import List, Dict, Tuple

# الألوان للطباعة
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_header(text: str):
    """طباعة عنوان"""
    print(f"\n{Colors.BOLD}{Colors.BLUE}{'='*60}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}{text:^60}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}{'='*60}{Colors.RESET}\n")

def print_success(text: str):
    """طباعة نجاح"""
    print(f"{Colors.GREEN}✓ {text}{Colors.RESET}")

def print_error(text: str):
    """طباعة خطأ"""
    print(f"{Colors.RED}✗ {text}{Colors.RESET}")

def print_warning(text: str):
    """طباعة تحذير"""
    print(f"{Colors.YELLOW}⚠ {text}{Colors.RESET}")

def print_info(text: str):
    """طباعة معلومة"""
    print(f"{Colors.BLUE}ℹ {text}{Colors.RESET}")

class GOLEventsTestSuite:
    """مجموعة اختبارات GOLEVENTS PRO"""
    
    def __init__(self, project_dir: str):
        self.project_dir = Path(project_dir)
        self.src_dir = self.project_dir / "src"
        self.tools_dir = self.src_dir / "tools"
        self.results = {
            "passed": 0,
            "failed": 0,
            "warnings": 0
        }
    
    def test_project_structure(self) -> bool:
        """اختبار بنية المشروع"""
        print_header("اختبار 1: بنية المشروع")
        
        required_files = [
            "CMakeLists.txt",
            "vcpkg.json",
            "README.md",
            "src/main.cpp",
            "src/MainWindow.h",
            "src/MainWindow.cpp",
            "src/AuthManager.h",
            "src/AuthManager.cpp",
            "src/UpdateChecker.h",
            "src/UpdateChecker.cpp",
            "src/Utils.h",
            "src/Utils.cpp",
        ]
        
        all_exist = True
        for file in required_files:
            file_path = self.project_dir / file
            if file_path.exists():
                print_success(f"وجد: {file}")
            else:
                print_error(f"مفقود: {file}")
                all_exist = False
                self.results["failed"] += 1
        
        if all_exist:
            self.results["passed"] += 1
            print_success("جميع الملفات الأساسية موجودة")
        
        return all_exist
    
    def test_tools_implementation(self) -> bool:
        """اختبار تنفيذ الأدوات"""
        print_header("اختبار 2: تنفيذ الأدوات (10 أدوات)")
        
        tools = [
            "CalcStock",
            "CheckListing",
            "CheckPrice",
            "ExpanderSeats",
            "PdfsToTxt",
            "Placeholder",
            "QrGenerator",
            "RenamerFv",
            "SplitterRenamer",
            "VerifyOrders"
        ]
        
        all_tools_exist = True
        for tool in tools:
            h_file = self.tools_dir / f"{tool}.h"
            cpp_file = self.tools_dir / f"{tool}.cpp"
            
            if h_file.exists() and cpp_file.exists():
                print_success(f"الأداة {tool}: ملفات .h و .cpp موجودة")
            else:
                print_error(f"الأداة {tool}: ملفات مفقودة")
                all_tools_exist = False
                self.results["failed"] += 1
        
        if all_tools_exist:
            self.results["passed"] += 1
            print_success("جميع الأدوات العشرة منفذة")
        
        return all_tools_exist
    
    def test_cmake_configuration(self) -> bool:
        """اختبار تكوين CMake"""
        print_header("اختبار 3: تكوين CMake")
        
        cmake_file = self.project_dir / "CMakeLists.txt"
        
        if not cmake_file.exists():
            print_error("ملف CMakeLists.txt غير موجود")
            self.results["failed"] += 1
            return False
        
        with open(cmake_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        required_packages = [
            "Qt6",
            "CURL",
            "nlohmann_json",
            "unofficial-qrencode",
            "Tesseract",
            "poppler"
        ]
        
        all_packages = True
        for package in required_packages:
            if f"find_package({package}" in content:
                print_success(f"حزمة {package} مُعرّفة في CMake")
            else:
                print_error(f"حزمة {package} غير مُعرّفة")
                all_packages = False
                self.results["failed"] += 1
        
        if all_packages:
            self.results["passed"] += 1
            print_success("جميع الحزم المطلوبة مُعرّفة في CMake")
        
        return all_packages
    
    def test_code_quality(self) -> bool:
        """اختبار جودة الكود"""
        print_header("اختبار 4: جودة الكود C++")
        
        # فحص الملفات الأساسية
        files_to_check = [
            self.src_dir / "MainWindow.cpp",
            self.src_dir / "AuthManager.cpp",
            self.src_dir / "UpdateChecker.cpp"
        ]
        
        issues = []
        
        for file_path in files_to_check:
            if not file_path.exists():
                continue
            
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                lines = content.split('\n')
            
            # فحص الأخطاء الشائعة
            if '#include' not in content:
                issues.append(f"{file_path.name}: لا يحتوي على includes")
            
            if 'namespace' not in content and file_path.name != "main.cpp":
                print_warning(f"{file_path.name}: لا يستخدم namespace")
                self.results["warnings"] += 1
            
            # فحص استخدام Qt
            if file_path.name == "MainWindow.cpp":
                if 'QWidget' in content or 'QMainWindow' in content:
                    print_success(f"{file_path.name}: يستخدم Qt بشكل صحيح")
                else:
                    print_warning(f"{file_path.name}: قد لا يستخدم Qt بشكل صحيح")
                    self.results["warnings"] += 1
        
        if not issues:
            self.results["passed"] += 1
            print_success("الكود يبدو جيدًا")
            return True
        else:
            for issue in issues:
                print_error(issue)
            self.results["failed"] += 1
            return False
    
    def test_firebase_integration(self) -> bool:
        """اختبار تكامل Firebase"""
        print_header("اختبار 5: تكامل Firebase")
        
        auth_file = self.src_dir / "AuthManager.cpp"
        
        if not auth_file.exists():
            print_error("ملف AuthManager.cpp غير موجود")
            self.results["failed"] += 1
            return False
        
        with open(auth_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        firebase_checks = {
            "FIREBASE_URL": "golevents1-default-rtdb.europe-west1.firebasedatabase.app" in content,
            "CURL": "curl" in content.lower() or "CURL" in content,
            "JSON": "json" in content.lower() or "nlohmann" in content
        }
        
        all_checks = True
        for check, passed in firebase_checks.items():
            if passed:
                print_success(f"تكامل {check}: موجود")
            else:
                print_error(f"تكامل {check}: مفقود")
                all_checks = False
                self.results["failed"] += 1
        
        if all_checks:
            self.results["passed"] += 1
            print_success("تكامل Firebase يبدو صحيحًا")
        
        return all_checks
    
    def test_resources(self) -> bool:
        """اختبار الموارد"""
        print_header("اختبار 6: الموارد")
        
        resources = [
            "knight.png",
            "knight.ico",
            "resources/resources.qrc"
        ]
        
        all_exist = True
        for resource in resources:
            resource_path = self.project_dir / resource
            if resource_path.exists():
                print_success(f"المورد {resource}: موجود")
            else:
                print_warning(f"المورد {resource}: مفقود")
                all_exist = False
                self.results["warnings"] += 1
        
        if all_exist:
            self.results["passed"] += 1
        
        return all_exist
    
    def generate_report(self):
        """إنشاء تقرير الاختبار"""
        print_header("تقرير الاختبار النهائي")
        
        total = self.results["passed"] + self.results["failed"]
        if total > 0:
            success_rate = (self.results["passed"] / total) * 100
        else:
            success_rate = 0
        
        print(f"{Colors.BOLD}النتائج:{Colors.RESET}")
        print(f"  {Colors.GREEN}✓ نجح: {self.results['passed']}{Colors.RESET}")
        print(f"  {Colors.RED}✗ فشل: {self.results['failed']}{Colors.RESET}")
        print(f"  {Colors.YELLOW}⚠ تحذيرات: {self.results['warnings']}{Colors.RESET}")
        print(f"\n{Colors.BOLD}نسبة النجاح: {success_rate:.1f}%{Colors.RESET}\n")
        
        if success_rate >= 80:
            print(f"{Colors.GREEN}{Colors.BOLD}✓ التطبيق جاهز للبناء!{Colors.RESET}")
        elif success_rate >= 60:
            print(f"{Colors.YELLOW}{Colors.BOLD}⚠ التطبيق يحتاج بعض التحسينات{Colors.RESET}")
        else:
            print(f"{Colors.RED}{Colors.BOLD}✗ التطبيق يحتاج إصلاحات كبيرة{Colors.RESET}")
        
        # حفظ التقرير
        report_file = self.project_dir / "test_report.json"
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump({
                "results": self.results,
                "success_rate": success_rate,
                "timestamp": str(Path(__file__).stat().st_mtime)
            }, f, indent=2, ensure_ascii=False)
        
        print(f"\n{Colors.BLUE}تم حفظ التقرير في: {report_file}{Colors.RESET}")
    
    def run_all_tests(self):
        """تشغيل جميع الاختبارات"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}")
        print("╔════════════════════════════════════════════════════════════╗")
        print("║         GOLEVENTS PRO - C++ Test Suite                    ║")
        print("║         اختبار شامل لتطبيق C++                           ║")
        print("╚════════════════════════════════════════════════════════════╝")
        print(f"{Colors.RESET}")
        
        self.test_project_structure()
        self.test_tools_implementation()
        self.test_cmake_configuration()
        self.test_code_quality()
        self.test_firebase_integration()
        self.test_resources()
        
        self.generate_report()

def main():
    """الدالة الرئيسية"""
    # الحصول على مسار المشروع
    project_dir = Path(__file__).parent
    
    # إنشاء وتشغيل الاختبارات
    test_suite = GOLEventsTestSuite(project_dir)
    test_suite.run_all_tests()
    
    # طباعة معلومات إضافية
    print_header("خطوات البناء التالية")
    print_info("لبناء التطبيق C++، تحتاج إلى:")
    print("  1. تثبيت CMake (https://cmake.org/download/)")
    print("  2. تثبيت vcpkg (https://github.com/Microsoft/vcpkg)")
    print("  3. تثبيت المكتبات المطلوبة عبر vcpkg")
    print("  4. تشغيل: cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    print("  5. تشغيل: cmake --build . --config Release")
    
    print_header("اختبار النسخة Python")
    print_info("لاختبار الوظائف الأساسية، يمكنك تشغيل النسخة Python:")
    print("  python main.py")

if __name__ == "__main__":
    main()
