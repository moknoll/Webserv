// Плавное появление блоков при загрузке страницы
window.addEventListener("load", () => {
    const elements = document.querySelectorAll(".fade-in");

    elements.forEach((el, index) => {
        setTimeout(() => {
            el.classList.add("show");
        }, index * 250);
    });
});

// Небольшой эффект изменения заголовка
const title = document.querySelector("h1");

title.addEventListener("mouseenter", () => {
    title.textContent = "🚀 Welcome to Webserv 🚀";
});

title.addEventListener("mouseleave", () => {
    title.textContent = "Welcome to Webserv";
});

// Кнопки немного подпрыгивают при клике
document.querySelectorAll(".btn").forEach(btn => {
    btn.addEventListener("click", () => {
        btn.style.transform = "scale(0.95)";

        setTimeout(() => {
            btn.style.transform = "";
        }, 150);
    });
});
