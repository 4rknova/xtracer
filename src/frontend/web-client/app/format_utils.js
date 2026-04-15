// Clock-style elapsed time: "MM:SS" or "HH:MM:SS".
function formatElapsed(ms) {
  const totalSeconds = Math.max(0, Math.floor((ms || 0) / 1000));
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  if (hours > 0) {
    return `${String(hours).padStart(2, "0")}:${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
  }
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

// Compact clock: always "MM:SS" with no hours component.
function formatElapsedShort(ms) {
  const totalSeconds = Math.max(0, Math.floor((ms || 0) / 1000));
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

// Human-readable elapsed time: "4.2s", "2m 05s", "1h 02m 05s".
// Returns "-" for zero/missing values.
function formatElapsedVerbose(ms) {
  if (!ms || ms <= 0) return "-";
  const totalSeconds = Math.floor(ms / 1000);
  if (totalSeconds < 60) return (ms / 1000).toFixed(1) + "s";
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  const mm = String(minutes).padStart(2, "0");
  const ss = String(seconds).padStart(2, "0");
  if (hours > 0) return `${hours}h ${mm}m ${ss}s`;
  return `${minutes}m ${ss}s`;
}

function formatBytesShort(bytes) {
  const n = Number(bytes) || 0;
  if (n < 1024) return `${n} B`;
  if (n < (1024 * 1024)) return `${(n / 1024).toFixed(1)} KB`;
  return `${(n / (1024 * 1024)).toFixed(2)} MB`;
}
