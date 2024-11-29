document.addEventListener('DOMContentLoaded', function() {
    // Komut gönderme fonksiyonu
    window.sendCommand = function(cmd) {
        fetch('/api/' + cmd, {
            method: 'POST'
        }).catch(console.error);
    };

    // Volume kontrolü
    const volumeSlider = document.getElementById('volume');
    if (volumeSlider) {
        volumeSlider.addEventListener('input', function() {
            let value = this.value;
            document.getElementById('volumeValue').textContent = value + '%';
            fetch('/api/volume', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'value=' + value
            }).catch(console.error);
        });
    }

    // MP3 listesini yükle
    function loadMusicList() {
        fetch('/api/playlist')
            .then(response => response.json())
            .then(files => {
                const musicList = document.getElementById('musicList');
                musicList.innerHTML = '';
                
                files.forEach(file => {
                    const li = document.createElement('li');
                    li.textContent = file;
                    li.onclick = function() {
                        fetch('/api/play', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ file: file })
                        }).catch(console.error);
                    };
                    musicList.appendChild(li);
                });
            })
            .catch(console.error);
    }

    // İlk durum güncellemesi ve periyodik güncelleme
    function updateStatus() {
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                if (data.track) {
                    document.getElementById('currentTrack').textContent = data.track;
                }
                if (data.temperature) {
                    document.getElementById('temperature').textContent = data.temperature;
                }
                if (data.wifi) {
                    document.getElementById('wifiStatus').textContent = data.wifi;
                }
                if (data.volume) {
                    document.getElementById('volume').value = data.volume;
                    document.getElementById('volumeValue').textContent = data.volume + '%';
                }
                if (data.bluetooth) {
                    document.getElementById('bluetoothStatus').textContent = data.bluetooth;
                }
            })
            .catch(console.error);
    }

    // İlk yükleme
    loadMusicList();
    updateStatus();
    
    // Periyodik güncelleme
    setInterval(updateStatus, 1000);
    setInterval(loadMusicList, 5000);  // MP3 listesini her 5 saniyede bir güncelle

    // Reset WiFi settings
    window.resetWiFi = function() {
        if (confirm('Are you sure you want to reset WiFi settings? Device will restart.')) {
            fetch('/api/reset-wifi', {
                method: 'POST'
            }).then(() => {
                alert('WiFi settings reset. Device will restart.');
            }).catch(console.error);
        }
    };

    // Clear NVS data
    window.clearNVS = function() {
        if (confirm('Are you sure you want to clear all saved data? This cannot be undone.')) {
            fetch('/api/clear-nvs', {
                method: 'POST'
            }).then(() => {
                alert('All data cleared. Device will restart.');
            }).catch(console.error);
        }
    };

    // NTP senkronizasyonu
    window.syncNTP = function() {
        fetch('/api/sync-time', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    alert('Time synchronized successfully!');
                } else {
                    alert('Time synchronization failed. Please check your internet connection.');
                }
                updateTimeDisplay();
            })
            .catch(error => {
                console.error(error);
                alert('Error synchronizing time');
            });
    };

    // Manuel saat ayarı
    window.setManualTime = function() {
        const dateTime = document.getElementById('manualDateTime').value;
        fetch('/api/set-time', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ datetime: dateTime })
        })
        .then(() => {
            alert('Time set successfully!');
            updateTimeDisplay();
        })
        .catch(console.error);
    };

    // Timer ekleme
    window.addTimer = function() {
        const dateTime = document.getElementById('timerDateTime').value;
        const action = document.getElementById('timerAction').value;
        
        fetch('/api/add-timer', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ 
                datetime: dateTime,
                action: action 
            })
        })
        .then(() => {
            alert('Timer added successfully!');
            loadTimers();
        })
        .catch(console.error);
    };

    // Timer listesini yükle
    function loadTimers() {
        fetch('/api/timers')
            .then(response => response.json())
            .then(data => {
                if (data && data.timers) {  // timers array'ini kontrol et
                    const timerList = document.getElementById('timerList');
                    timerList.innerHTML = '';
                    
                    data.timers.forEach(timer => {
                        const div = document.createElement('div');
                        div.className = 'timer-list-item';
                        div.innerHTML = `
                            <span>${timer.datetime} - ${timer.action}</span>
                            <button onclick='deleteTimer("${timer.id}")'>
                                <i class='fas fa-trash'></i>
                            </button>
                        `;
                        timerList.appendChild(div);
                    });
                }
            })
            .catch(console.error);
    }

    // Saat gösterimini güncelle
    function updateTimeDisplay() {
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                // RTC zamanı
                if (data.time) {
                    const timeStr = `${String(data.time.hour).padStart(2, '0')}:${String(data.time.minute).padStart(2, '0')}:${String(data.time.second).padStart(2, '0')}`;
                    document.getElementById('currentTime').textContent = timeStr;
                    
                    if (data.date) {
                        const dateStr = `${data.date.year}-${String(data.date.month).padStart(2, '0')}-${String(data.date.day).padStart(2, '0')}`;
                        document.getElementById('currentDate').textContent = dateStr;
                    }
                }
                
                // NTP zamanı
                if (data.ntp) {
                    const ntpStr = `${String(data.ntp.hour).padStart(2, '0')}:${String(data.ntp.minute).padStart(2, '0')}:${String(data.ntp.second).padStart(2, '0')} UTC${data.timezone >= 0 ? '+' : ''}${data.timezone}`;
                    document.getElementById('ntpTime').textContent = ntpStr;
                }
            })
            .catch(console.error);
    }

    // Timer listesini yükle
    loadTimers();
    
    // Saat gösterimini periyodik güncelle
    setInterval(updateTimeDisplay, 1000);

    function deleteTimer(timerId) {
        if (confirm('Are you sure you want to delete this timer?')) {
            fetch('/api/remove-timer', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'id=' + timerId
            })
            .then(() => {
                loadTimers();  // Timer listesini yenile
            })
            .catch(console.error);
        }
    }

    // Timezone ayarı
    window.setTimezone = function() {
        const offset = document.getElementById('utcOffset').value;
        fetch('/api/set-timezone', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ offset: parseInt(offset) })
        })
        .then(() => {
            updateTimeDisplay();
        })
        .catch(console.error);
    };

    // Dosya yükleme fonksiyonu
    window.uploadFiles = function() {
        const fileInput = document.getElementById('fileInput');
        const files = fileInput.files;
        const progressDiv = document.getElementById('uploadProgress');
        
        if (files.length === 0) {
            alert('Please select files to upload');
            return;
        }

        progressDiv.innerHTML = '';
        
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            const formData = new FormData();
            formData.append('file', file);
            
            const progressBar = document.createElement('div');
            progressBar.className = 'progress-bar';
            progressBar.innerHTML = `<span>${file.name}: 0%</span>`;
            progressDiv.appendChild(progressBar);
            
            fetch('/api/upload', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    progressBar.innerHTML = `<span>${file.name}: Completed!</span>`;
                    progressBar.classList.add('complete');
                    loadMusicList(); // Listeyi güncelle
                } else {
                    progressBar.innerHTML = `<span>${file.name}: Failed!</span>`;
                    progressBar.classList.add('failed');
                }
            })
            .catch(error => {
                progressBar.innerHTML = `<span>${file.name}: Error!</span>`;
                progressBar.classList.add('failed');
            });
        }
    };
}); 