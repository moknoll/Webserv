#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import cgitb
import os
import sys
import shutil
from datetime import datetime

# Включаем отладку для CGI
cgitb.enable()

# Настройки
UPLOAD_DIR = "./uploads"  # Директория для загрузки
MAX_FILE_SIZE = 100 * 1024 * 1024  # 10 MB
ALLOWED_EXTENSIONS = {'.txt', '.pdf', '.jpg', '.jpeg', '.png', '.gif', '.zip', '.doc', '.docx'}

def create_upload_dir():
    """Создает директорию для загрузки, если её нет"""
    if not os.path.exists(UPLOAD_DIR):
        os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)

def get_safe_filename(filename):
    """Очищает имя файла от опасных символов"""
    # Удаляем пути и опасные символы
    filename = os.path.basename(filename)
    # Заменяем пробелы на подчеркивания
    filename = filename.replace(' ', '_')
    # Удаляем все, кроме букв, цифр, точек, дефисов и подчеркиваний
    safe_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
    filename = ''.join(c for c in filename if c in safe_chars)
    return filename

def is_allowed_file(filename):
    """Проверяет, разрешен ли тип файла"""
    ext = os.path.splitext(filename)[1].lower()
    return ext in ALLOWED_EXTENSIONS

def get_file_size(file_obj):
    """Получает размер файла"""
    file_obj.seek(0, os.SEEK_END)
    size = file_obj.tell()
    file_obj.seek(0)
    return size

def print_html_header(title="Загрузка файла"):
    """Выводит HTML заголовок"""
    print("Content-Type: text/html; charset=utf-8")
    print()
    print(f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{title}</title>
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
    <h1>📤 Загрузка файла</h1>
""")

def print_html_footer():
    """Выводит HTML подвал"""
    print("""
</body>
</html>
""")

def show_upload_form(message=""):
    """Показывает форму загрузки"""
    print_html_header()
    
    if message:
        print(f'<div class="info">{message}</div>')
    
    print(f"""
    <form method="post" enctype="multipart/form-data" action="{os.environ.get('SCRIPT_NAME', '')}">
        <div class="form-group">
            <label for="file">Выберите файл:</label>
            <input type="file" name="file" id="file" required>
        </div>
        <div class="form-group">
            <label for="description">Описание (необязательно):</label>
            <input type="text" name="description" id="description" placeholder="Краткое описание файла">
        </div>
        <input type="submit" value="📤 Загрузить">
    </form>
    
    <div class="file-list">
        <h3>📁 Загруженные файлы:</h3>
        <p><em>Проверьте директорию: {UPLOAD_DIR}</em></p>
    </div>
    """)
    
    print_html_footer()

def show_result(success, message, filename=""):
    """Показывает результат загрузки"""
    print_html_header("Результат загрузки")
    
    if success:
        print(f'<div class="success">✅ {message}</div>')
        if filename:
            print(f'<p><strong>Файл:</strong> {filename}</p>')
            print(f'<p><strong>Путь:</strong> {os.path.join(UPLOAD_DIR, filename)}</p>')
    else:
        print(f'<div class="error">❌ {message}</div>')
    
    print('<p><a href="javascript:history.back()">← Назад</a></p>')
    print_html_footer()

def handle_upload():
    """Обрабатывает загрузку файла"""
    try:
        # Создаем директорию для загрузки
        create_upload_dir()
        
        # Получаем данные формы
        form = cgi.FieldStorage()
        
        # Проверяем, был ли загружен файл
        if 'file' not in form:
            return False, "Файл не выбран", ""
        
        file_item = form['file']
        
        # Проверяем, что это файл
        if not file_item.filename:
            return False, "Имя файла не указано", ""
        
        # Получаем имя файла
        original_filename = file_item.filename
        safe_filename = get_safe_filename(original_filename)
        
        if not safe_filename:
            return False, "Некорректное имя файла", ""
        
        # Проверяем тип файла
        if not is_allowed_file(safe_filename):
            return False, f"Тип файла не разрешен. Разрешенные: {', '.join(ALLOWED_EXTENSIONS)}", ""
        
        # Проверяем размер
        if file_item.file:
            file_size = get_file_size(file_item.file)
            if file_size > MAX_FILE_SIZE:
                return False, f"Файл слишком большой. Максимальный размер: {MAX_FILE_SIZE // (1024*1024)} MB", ""
        
        # Генерируем уникальное имя, если файл уже существует
        base, ext = os.path.splitext(safe_filename)
        counter = 1
        final_filename = safe_filename
        
        while os.path.exists(os.path.join(UPLOAD_DIR, final_filename)):
            final_filename = f"{base}_{counter}{ext}"
            counter += 1
        
        # Сохраняем файл
        file_path = os.path.join(UPLOAD_DIR, final_filename)
        
        # Копируем содержимое
        with open(file_path, 'wb') as f:
            shutil.copyfileobj(file_item.file, f)
        
        # Устанавливаем права
        os.chmod(file_path, 0o644)
        
        # Получаем описание
        description = form.getvalue('description', '')
        
        # Логируем загрузку
        log_entry = f"{datetime.now().isoformat()} | {final_filename} | {file_size} bytes | {description}\n"
        log_file = os.path.join(UPLOAD_DIR, "upload.log")
        with open(log_file, 'a', encoding='utf-8') as f:
            f.write(log_entry)
        
        success_msg = f"Файл успешно загружен! Размер: {file_size} байт"
        if description:
            success_msg += f" | Описание: {description}"
        
        return True, success_msg, final_filename
        
    except Exception as e:
        return False, f"Ошибка при загрузке: {str(e)}", ""

def list_uploaded_files():
    """Список загруженных файлов"""
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

# ============ ОСНОВНАЯ ЛОГИКА ============

def main():
    """Основная функция CGI скрипта"""
    
    # Проверяем метод запроса
    if os.environ.get('REQUEST_METHOD') == 'POST':
        # Обрабатываем загрузку
        success, message, filename = handle_upload()
        show_result(success, message, filename)
    else:
        # Показываем форму для GET запроса
        show_upload_form()

# Запускаем скрипт
if __name__ == "__main__":
    main()
