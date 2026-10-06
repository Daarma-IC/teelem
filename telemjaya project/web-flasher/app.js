const row = document.querySelector(".status-row");
const label = document.querySelector("#browser-status");

if ("serial" in navigator && window.isSecureContext) {
  row.classList.add("ready");
  label.textContent = "Web Serial siap · sambungkan LilyGO via USB";
} else {
  row.classList.add("error");
  label.textContent = "Gunakan Chrome/Edge melalui localhost atau HTTPS";
}
