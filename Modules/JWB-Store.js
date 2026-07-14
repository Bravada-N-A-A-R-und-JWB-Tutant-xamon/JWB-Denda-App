.pragma library
.import QtQuick.LocalStorage 2.12 as Sql

console.warn("JWB-Denda LiveScript at live")

// =========================================================================
// 1. КОНСТАНТЫ И КОНФИГУРАЦИЯ СИСТЕМЫ
// =========================================================================
const KING = "Khmelyauskas-Alexander-Eduardovich";
const URL_BASE = "https://github.com/orgs/Bravada-N-A-A-R-und-JWB-Tutant-xamon";

// Состояния текущей сессии юзера
let currentUser = "Guest";
let userRole = "user"; // Может быть: user, admin, KING
let isMuted = false;

// =========================================================================
// 2. СИСТЕМА ДИНАМИЧЕСКИХ ТЕМ (ЦВЕТА)
// =========================================================================
var THEMES = [
    { bg: "#07111E", card: "#0F1C2E", banner: "#074c13", accent: "#00FFCC", order: "1", name: "GreenMarine", id: "bluegreen_marine" },
    { bg: "#0F0500", card: "#1A0B05", banner: "#a84c05", accent: "#FF6600", order: "2", name: "Rebel", id: "rebelRedTheme" },
    { bg: "#0A000F", card: "#140220", banner: "#7900b3", accent: "#DF00FF", order: "3", name: "Catala Kyoshiki Murasaki de CiberSheep / Purple Light", id: "cs_light"},
    { bg: "#0F0000", card: "#1F0505", banner: "#790000", accent: "#FF3333", order: "4", name: "Very yummi chips", id: "orange_yummi_chips" }
];

function getRandomTheme() {
    var chosenTheme = THEMES[Math.floor(Math.random() * THEMES.length)];

    // Теперь читаем свойства ИМЕННО из выбранной темы:
    console.log("Theme triggered: " + chosenTheme.name +
		" | Theme's ID: " + chosenTheme.id +
		" | Theme's Order is: " + chosenTheme.order);

    return chosenTheme;
}

// =========================================================================
// 3. РЕЖИМ КОРОЛЯ (КОНТРОЛЬ ДОСТУПА И МОДЕРАЦИЯ)
// =========================================================================
function initializeSession(username) {
    currentUser = username;
    if (username === KING) {
        userRole = "KING";
        console.log(">>> Доступ уровня КОРОЛЬ активирован. Приветствуем, Александр!");
    } else {
        userRole = "user";
    }
}

function checkPermission(requiredRole) {
    if (userRole === "KING") return true; // Королю можно всё
    if (requiredRole === "admin" && userRole === "admin") return true;
    return false;
}

// Функции модерации (доступны только KING)
function moderateUser(targetUser, action) {
    if (!checkPermission("admin")) {
        console.log("Ошибка: Недостаточно прав для выполнения операции.");
        return false;
    }

    // Действия: "ban", "mute", "promote_admin"
    console.log("Модерация юзера " + targetUser + " -> Действие: " + action);
    return true;
}

// =========================================================================
// 4. УПРАВЛЕНИЕ РЕВИЗИЯМИ И ВЫКЛАДКАМИ
// =========================================================================
function publishRevision(repoName, version, changelog) {
    console.log("Подготовка ревизии для " + repoName + " [v" + version + "]");
    // Тут будет генерация или отправка метаданных манифеста
}

// =========================================================================
// 5. МОДУЛЬ ОТЗЫВОВ И РЕЦЕНЗИЙ
// =========================================================================
function leaveReview(appId, rating, commentText) {
    if (isMuted) return { success: false, error: "Вы не можете оставлять отзывы (Muted)" };

    var reviewPayload = {
        "app": appId,
        "author": currentUser,
        "rating": rating,
        "text": commentText,
        "timestamp": Date.now()
    };

    console.log("Отзыв сформирован:", JSON.stringify(reviewPayload));
    return { success: true, data: reviewPayload };
}

// =========================================================================
// 6. БУДУЩИЙ МЕССЕНДЖЕР (ЗАДЕЛ ПОД СЕТЬ)
// =========================================================================
function sendMessage(recipient, messageText) {
    if (isMuted) return false;
    console.log("Отправка сообщения для " + recipient + ": " + messageText);
    // Сюда встанет вызов WebSocket или API-запрос к чат-серверу
}
