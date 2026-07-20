#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import cgitb
import os
import sys
import shutil
from datetime import datetime

# Enable debugging for CGI
cgitb.enable()

# Settings
UPLOAD_DIR = "./uploads"  # Directory for uploads
MAX_FILE_SIZE = 900 * 1024 * 1024
ALLOWED_EXTENSIONS = {'.txt', '.pdf', '.jpg', '.jpeg', '.png', '.gif', '.zip', '.doc', '.docx', '.txz', ".mp4", ".tar", ".gz", ".tar.gz", ".7z", ".rar", ".mpeg"}

def create_upload_dir():
    """Creates the upload directory if it does not exist"""
    if not os.path.exists(UPLOAD_DIR):
        os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)

def get_safe_filename(filename):
    """Cleans the filename of dangerous characters"""
    # Remove paths and dangerous characters
    filename = os.path.basename(filename)
    # Replace spaces with underscores
    filename = filename.replace(' ', '_')
    # Remove everything except letters, numbers, dots, hyphens, and underscores
    safe_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
    filename = ''.join(c for c in filename if c in safe_chars)
    return filename

def is_allowed_file(filename):
    """Checks if the file type is allowed"""
    ext = os.path.splitext(filename)[1].lower()
    return ext in ALLOWED_EXTENSIONS

def get_file_size(file_obj):
    """Gets the file size"""
    file_obj.seek(0, os.SEEK_END)
    size = file_obj.tell()
    file_obj.seek(0)
    return size

def print_html_header(title="Upload file"):
    """Outputs the HTML header"""
    print("Content-Type: text/html; charset=utf-8")
    print()
    print(f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{title}</title>
    <link rel="stylesheet" href="/py-scripts/style.css">
    <style>
        body {{ font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }}
        .success {{ color: green; background: #d4edda; padding: 15px; border-radius: 5px; }}
        .error {{ color: red; background: #f8d7da; padding: 15px; border-radius: 5px; }}
        .info {{ background: #d1ecf1; padding: 15px; border-radius: 5px; }}
        .form-group {{ margin-bottom: 15px; }}
        label {{ display: inline-block; margin-bottom: 5px; font-weight: bold; }}
        input[type="file"] {{ padding: 10px; border: 1px solid #ccc; border-radius: 4px; width: 100%; }}
        input[type="submit"] {{ background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }}
        input[type="submit"]:hover {{ background: #0056b3; }}
        .file-list {{ margin-top: 20px; }}
        .file-item {{ padding: 5px 0; border-bottom: 1px solid #eee; }}
    </style>
</head>
    
<body>

<div class="background">
    <nav class="navbar">
		<div class="logo">
			<a href="/">Webserv</a>
		</div>
		<ul class="nav-links">
			<li><a href="/cgi-bin/py/helloCGI.py">Home</a></li>
			<li><a href="/cgi-bin/py/upload.py">Upload</a></li>
			<li><a href="/cgi-bin/py/form-handler.py">Form handler</a></li>
			<li><a href="/cgi-bin/py/session_management.py">Session management</a></li>
			<li><a href="https://github.com/">GitHub</a></li>
		</ul>
	</nav>
</div>
    <h1>📤 Upload file</h1>
""")

def print_html_footer():
    """Outputs the HTML footer"""
    print("""
</body>
</html>
""")

def show_upload_form(message=""):
    """Shows the upload form"""
    print_html_header()
    
    if message:
        print(f'<div class="info">{message}</div>')
    
    print(f"""
    <form method="post" enctype="multipart/form-data" action="{os.environ.get('SCRIPT_NAME', '')}">
        <div class="form-group">
            <label for="file">Select file:</label>
            <input type="file" name="file" id="file" required>
        </div>
        <div class="form-group">
            <label for="description">Description (optional):</label>
            <input type="text" name="description" id="description" placeholder="Short file description">
        </div>
        <input type="submit" value="📤 Upload">
    </form>
    
    <div class="file-list">
        <h3>📁 Uploaded files:</h3>
        <p><em>Check directory: {UPLOAD_DIR}</em></p>
    </div>
    """)
    
    print_html_footer()

def show_result(success, message, filename=""):
    """Shows the upload result"""
    print_html_header("Uploaded result")
    
    if success:
        print(f'<div class="success">✅ {message}</div>')
        if filename:
            print(f'<p><strong>File:</strong> {filename}</p>')
            print(f'<p><strong>Path:</strong> {os.path.join(UPLOAD_DIR, filename)}</p>')
    else:
        print(f'<div class="error">❌ {message}</div>')
    
    print('<p><a href="javascript:history.back()">← Back</a></p>')
    print_html_footer()

def handle_upload():
    """Handles file upload"""
    try:
        # Create upload directory
        create_upload_dir()
        
        # Get form data
        form = cgi.FieldStorage()
        
        # Check if file was uploaded
        if 'file' not in form:
            return False, "No file selected", ""
        
        file_item = form['file']
        
        # Check that it is a file
        if not file_item.filename:
            return False, "Filename not specified", ""
        
        # Get filename
        original_filename = file_item.filename
        safe_filename = get_safe_filename(original_filename)
        
        if not safe_filename:
            return False, "Invalid filename", ""
        
        # Check file type
        if not is_allowed_file(safe_filename):
            return False, f"File type not allowed. Allowed: {', '.join(ALLOWED_EXTENSIONS)}", ""
        
        # Check size
        if file_item.file:
            file_size = get_file_size(file_item.file)
            if file_size > MAX_FILE_SIZE:
                return False, f"File too large. Maximum size: {MAX_FILE_SIZE // (1024*1024)} MB", ""
        
        # Generate a unique name if file already exists
        base, ext = os.path.splitext(safe_filename)
        counter = 1
        final_filename = safe_filename
        
        while os.path.exists(os.path.join(UPLOAD_DIR, final_filename)):
            final_filename = f"{base}_{counter}{ext}"
            counter += 1
        
        # Save file
        file_path = os.path.join(UPLOAD_DIR, final_filename)
        
        # Copy contents
        with open(file_path, 'wb') as f:
            shutil.copyfileobj(file_item.file, f)
        
        # Set permissions
        os.chmod(file_path, 0o644)
        
        # Get description
        description = form.getvalue('description', '')
        
        # Log upload
        log_entry = f"{datetime.now().isoformat()} | {final_filename} | {file_size} bytes | {description}\n"
        log_file = os.path.join(UPLOAD_DIR, "upload.log")
        with open(log_file, 'a', encoding='utf-8') as f:
            f.write(log_entry)
        
        success_msg = f"File successfully uploaded! Size: {file_size} bytes"
        if description:
            success_msg += f" | Description: {description}"
        
        return True, success_msg, final_filename
        
    except Exception as e:
        return False, f"Upload error: {str(e)}", ""


def list_uploaded_files():
    """List of uploaded files"""
    try:
        if not os.path.exists(UPLOAD_DIR):
            return []
        
        files = []
        for filename in os.listdir(UPLOAD_DIR):
            if filename != "upload.log":
                filepath = os.path.join(UPLOAD_DIR, filename)
                if os.path.isfile(filepath):
                    stat = os.stat(filepath)
                    files.append({
                        'name': filename,
                        'size': stat.st_size,
                        'modified': datetime.fromtimestamp(stat.st_mtime).strftime('%Y-%m-%d %H:%M:%S')
                    })
        return sorted(files, key=lambda x: x['modified'], reverse=True)
    except:
        return []

# ============ MAIN LOGIC ============

def main():
    """Main function of the CGI script"""
    
    # Check request method
    if os.environ.get('REQUEST_METHOD') == 'POST':
        # Handle upload
        success, message, filename = handle_upload()
        show_result(success, message, filename)
    else:
        # Show form for GET request
        show_upload_form()

# Run the script
if __name__ == "__main__":
    main()
