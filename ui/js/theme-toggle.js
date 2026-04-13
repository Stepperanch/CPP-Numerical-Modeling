(function () {
    var storageKey = "site-theme";
    var mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");

    function getStoredTheme() {
        var value = localStorage.getItem(storageKey);
        if (value === "light" || value === "dark" || value === "auto") {
            return value;
        }
        return "auto";
    }

    function getEffectiveTheme(themeChoice) {
        if (themeChoice === "light" || themeChoice === "dark") {
            return themeChoice;
        }
        return mediaQuery.matches ? "dark" : "light";
    }

    function applyTheme(themeChoice) {
        var effectiveTheme = getEffectiveTheme(themeChoice);
        document.documentElement.setAttribute("data-theme", effectiveTheme);
        document.documentElement.style.colorScheme = effectiveTheme;
    }

    function persistTheme(themeChoice) {
        localStorage.setItem(storageKey, themeChoice);
    }

    function buildControl(currentChoice) {
        if (document.querySelector(".theme-control")) {
            return;
        }

        var container = document.createElement("div");
        container.className = "theme-control";

        var label = document.createElement("label");
        label.setAttribute("for", "theme-select");
        label.textContent = "Theme";

        var select = document.createElement("select");
        select.id = "theme-select";
        select.setAttribute("aria-label", "Choose theme");

        [
            { value: "auto", text: "Auto" },
            { value: "light", text: "Light" },
            { value: "dark", text: "Dark" }
        ].forEach(function (item) {
            var option = document.createElement("option");
            option.value = item.value;
            option.textContent = item.text;
            if (item.value === currentChoice) {
                option.selected = true;
            }
            select.appendChild(option);
        });

        select.addEventListener("change", function () {
            var nextChoice = select.value;
            persistTheme(nextChoice);
            applyTheme(nextChoice);
        });

        container.appendChild(label);
        container.appendChild(select);
        document.body.appendChild(container);
    }

    function initializeTheme() {
        var currentChoice = getStoredTheme();
        applyTheme(currentChoice);

        if (currentChoice === "auto") {
            mediaQuery.addEventListener("change", function () {
                applyTheme("auto");
            });
        }

        if (document.readyState === "loading") {
            document.addEventListener("DOMContentLoaded", function () {
                buildControl(currentChoice);
            });
        } else {
            buildControl(currentChoice);
        }
    }

    initializeTheme();
})();
