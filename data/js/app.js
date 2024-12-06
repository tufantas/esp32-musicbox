<<<<<<< HEAD
document.addEventListener('DOMContentLoaded', function() {
    // Komut gönderme fonksiyonu
    window.sendCommand = function(cmd) {
        fetch('/api/' + cmd, {
            method: 'POST'
        }).catch(console.error);
=======
const getBaseUrl = () => {
    return window.location.origin;
};

document.addEventListener('DOMContentLoaded', function() {
    // Komut gönderme fonksiyonu
    window.sendCommand = function(cmd) {
        fetch(getBaseUrl() + '/api/' + cmd, {
            method: 'POST'
        }).catch(error => {
            console.error('Command error:', error);
            // Bağlantı hatası durumunda sayfayı yenile
            if (error.message.includes('Failed to fetch')) {
                setTimeout(() => {
                    window.location.reload();
                }, 2000);
            }
        });
>>>>>>> stable-power-audio
    };

    // Volume kontrolü
    const volumeSlider = document.getElementById('volume');
    if (volumeSlider) {
<<<<<<< HEAD
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
=======
        let volumeTimeout;
        let lastVolume = volumeSlider.value;
        
        volumeSlider.addEventListener('input', function() {
            let value = this.value;
            document.getElementById('volumeValue').textContent = value + '%';
            
            // Değişim çok küçükse güncelleme yapma
            if (Math.abs(value - lastVolume) < 2) {
                return;
            }
            
            // Debounce volume güncellemelerini
            clearTimeout(volumeTimeout);
            volumeTimeout = setTimeout(() => {
                fetch('/api/volume', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: 'value=' + value
                })
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`HTTP error! status: ${response.status}`);
                    }
                    lastVolume = value;
                })
                .catch(error => {
                    console.error('Volume update error:', error);
                    // Hata durumunda eski değere geri dön
                    volumeSlider.value = lastVolume;
                    document.getElementById('volumeValue').textContent = lastVolume + '%';
                });
            }, 100);  // 100ms bekle
>>>>>>> stable-power-audio
        });
    }

    // MP3 listesini yükle
    function loadMusicList() {
<<<<<<< HEAD
        fetch('/api/playlist')
=======
        fetch(getBaseUrl() + '/api/playlist')
>>>>>>> stable-power-audio
            .then(response => response.json())
            .then(files => {
                const musicList = document.getElementById('musicList');
                musicList.innerHTML = '';
                
                files.forEach(file => {
                    const li = document.createElement('li');
<<<<<<< HEAD
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
=======
                    
                    // Dosya bilgisi için div
                    const fileInfo = document.createElement('div');
                    fileInfo.className = 'file-info';
                    
                    // Dosya formatına göre ikon ekle
                    const ext = file.toLowerCase().split('.').pop();
                    let icon = '';
                    if (ext === 'mp3') {
                        icon = '<i class="fas fa-music" style="color: #4CAF50;"></i>';
                    } else if (ext === 'm4a') {
                        icon = '<i class="fas fa-music" style="color: #2196F3;"></i>';
                    }
                    fileInfo.innerHTML = `${icon} ${file}`;
                    
                    // Silme butonu
                    const deleteBtn = document.createElement('button');
                    deleteBtn.className = 'delete-btn';
                    deleteBtn.innerHTML = '<i class="fas fa-trash"></i>';
                    
                    // Silme işlemi
                    deleteBtn.onclick = function(e) {
                        e.stopPropagation(); // Çalma işlemini engelle
                        
                        if (confirm(`"${file}" dosyasını silmek istediğinize emin misiniz?`)) {
                            fetch('/api/delete', {
                                method: 'POST',
                                headers: {
                                    'Content-Type': 'application/x-www-form-urlencoded'
                                },
                                body: 'file=' + encodeURIComponent(file)
                            })
                            .then(response => {
                                if (!response.ok) {
                                    throw new Error(`HTTP error! status: ${response.status}`);
                                }
                                loadMusicList(); // Listeyi yenile
                            })
                            .catch(error => {
                                console.error('Error:', error);
                                alert(`Dosya silinemedi: ${file}`);
                            });
                        }
                    };
                    
                    // Çalma işlemi
                    fileInfo.onclick = function() {
                        console.log('Playing:', file);
                        fetch('/api/play', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/x-www-form-urlencoded'
                            },
                            body: 'file=' + encodeURIComponent(file)
                        })
                        .then(response => {
                            if (!response.ok) {
                                throw new Error(`HTTP error! status: ${response.status}`);
                            }
                            console.log('Play request sent successfully');
                            // Şarkı başladığında play/stop butonunu güncelle
                            isPlaying = true;
                            const playStopBtn = document.getElementById('playStopBtn');
                            if (playStopBtn) {  // Null kontrolü ekle
                                const icon = playStopBtn.querySelector('i');
                                if (icon) {  // Null kontrolü ekle
                                    icon.className = 'fas fa-stop';
                                }
                            }
                        })
                        .catch(error => {
                            console.error('Error:', error);
                            alert(`Çalma hatası: ${file}`);
                        });
                    };
                    
                    li.appendChild(fileInfo);
                    li.appendChild(deleteBtn);
                    musicList.appendChild(li);
                });
            })
            .catch(error => {
                console.error('Playlist error:', error);
                if (error.message.includes('Failed to fetch')) {
                    setTimeout(() => {
                        window.location.reload();
                    }, 2000);
                }
            });
    }

    // Şarkı ilerleme kontrolü
    let trackDuration = 0;
    let currentPosition = 0;

    function updateTrackProgress() {
        if (trackDuration > 0) {
            const percent = (currentPosition / trackDuration) * 100;
            document.getElementById('trackProgressBar').style.width = percent + '%';
            
            // Süre gösterimini güncelle
            document.getElementById('trackCurrentTime').textContent = formatTime(currentPosition);
            document.getElementById('trackDuration').textContent = formatTime(trackDuration);
        }
    }

    function formatTime(seconds) {
        const minutes = Math.floor(seconds / 60);
        const remainingSeconds = Math.floor(seconds % 60);
        return `${minutes}:${remainingSeconds.toString().padStart(2, '0')}`;
    }

    // Play/Stop durumu
    let isPlaying = false;

    // Play/Stop toggle fonksiyonu
    window.togglePlayStop = function() {
        const playStopBtn = document.getElementById('playStopBtn');
        const icon = playStopBtn.querySelector('i');
        
        if (isPlaying) {
            // Stop
            fetch('/api/stop', {
                method: 'POST'
            })
            .then(response => {
                if (!response.ok) throw new Error('Stop failed');
                isPlaying = false;
                icon.className = 'fas fa-play';
            })
            .catch(error => {
                console.error('Error:', error);
            });
        } else {
            // Play
            fetch('/api/play', {
                method: 'POST'
            })
            .then(response => {
                if (!response.ok) throw new Error('Play failed');
                isPlaying = true;
                icon.className = 'fas fa-stop';
            })
            .catch(error => {
                console.error('Error:', error);
            });
        }
    };

    // Loop durumu
    let isLooping = false;

    // Loop toggle fonksiyonu
    window.toggleLoop = function() {
        const loopBtn = document.getElementById('loopBtn');
        isLooping = !isLooping;
        
        fetch('/api/loop', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: 'enabled=' + isLooping
        })
        .then(response => {
            if (!response.ok) throw new Error('Loop toggle failed');
            loopBtn.classList.toggle('active', isLooping);
        })
        .catch(error => {
            console.error('Error:', error);
            // Hata durumunda geri al
            isLooping = !isLooping;
            loopBtn.classList.toggle('active', isLooping);
        });
    };

    // İlk durum güncellemesi ve periyodik güncelleme
    function updateStatus() {
        fetch(getBaseUrl() + '/api/status')
>>>>>>> stable-power-audio
            .then(response => response.json())
            .then(data => {
                if (data.track) {
                    document.getElementById('currentTrack').textContent = data.track;
                }
                if (data.temperature) {
                    document.getElementById('temperature').textContent = data.temperature;
                }
                if (data.wifi) {
<<<<<<< HEAD
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
=======
                    document.getElementById('wifiStatus').textContent = 
                        data.wifi === "Connected" ? "Bağlı" : "Bağlı Değil";
                }
                // Volume güncellemesini sadece değer değişmişse yap
                if (data.volume && Math.abs(volumeSlider.value - data.volume) > 1) {
                    volumeSlider.value = data.volume;
                    document.getElementById('volumeValue').textContent = data.volume + '%';
                }
                // Saat ve tarih güncellemesi
                if (data.time) {
                    const hour = String(data.time.hour).padStart(2, '0');
                    const minute = String(data.time.minute).padStart(2, '0');
                    const second = String(data.time.second).padStart(2, '0');
                    document.getElementById('currentTime').textContent = 
                        `${hour}:${minute}:${second}`;
                    
                    if (data.time.date) {
                        const day = String(data.time.date.day).padStart(2, '0');
                        const month = String(data.time.date.month).padStart(2, '0');
                        const year = data.time.date.year;
                        document.getElementById('currentDate').textContent = 
                            `${day}/${month}/${year}`;
                    }
                }
                // Şarkı ilerleme bilgisini güncelle
                if (data.track_position) {
                    currentPosition = data.track_position;
                    trackDuration = data.track_duration || 0;
                    updateTrackProgress();
                }
                // Play durumunu güncelle
                const playStopBtn = document.getElementById('playStopBtn');
                const icon = playStopBtn.querySelector('i');
                
                if (data.playing) {
                    isPlaying = true;
                    icon.className = 'fas fa-stop';
                } else {
                    isPlaying = false;
                    icon.className = 'fas fa-play';
                }
                // Loop durumunu güncelle
                const loopBtn = document.getElementById('loopBtn');
                if (data.looping) {
                    isLooping = true;
                    loopBtn.classList.add('active');
                } else {
                    isLooping = false;
                    loopBtn.classList.remove('active');
                }
            })
            .catch(error => {
                console.error('Status error:', error);
                if (error.message.includes('Failed to fetch')) {
                    setTimeout(() => {
                        window.location.reload();
                    }, 2000);
                }
            });
>>>>>>> stable-power-audio
    }

    // İlk yükleme
    loadMusicList();
    updateStatus();
    
    // Periyodik güncelleme
<<<<<<< HEAD
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
=======
    setInterval(updateStatus, 2000);  // 2 saniyede bir güncelle
    setInterval(loadMusicList, 5000);  // MP3 listesini her 5 saniyede bir güncelle

    // Dosya yükleme işlemleri
    const uploadForm = document.getElementById('uploadForm');
    const fileInput = document.getElementById('fileInput');
    const progressBar = document.getElementById('progressBar');
    const progressText = document.getElementById('progressText');
    const uploadProgress = document.getElementById('uploadProgress');

    uploadForm.onsubmit = async function(e) {
        e.preventDefault();
        
        const files = fileInput.files;
        if (files.length === 0) {
            alert('Lütfen bir müzik dosyası seçin');
            return;
        }

        // Dosya uzantılarını kontrol et
        const validExtensions = ['.mp3', '.m4a', '.aac', '.wav'];
        for (let file of files) {
            const ext = file.name.substring(file.name.lastIndexOf('.')).toLowerCase();
            if (!validExtensions.includes(ext)) {
                alert(`Desteklenmeyen dosya formatı: ${file.name}\nDesteklenen formatlar: MP3, M4A, AAC, WAV`);
                return;
            }
        }

        uploadProgress.style.display = 'block';
        
        for (let i = 0; i < files.length; i++) {
            const formData = new FormData();
            formData.append('file', files[i]);
            
            try {
                const response = await fetch('/upload', {
                    method: 'POST',
                    body: formData
                });

                if (!response.ok) {
                    const errorText = await response.text();
                    throw new Error(errorText);
                }

                progressBar.style.width = `${((i + 1) / files.length) * 100}%`;
                progressText.textContent = `${Math.round(((i + 1) / files.length) * 100)}%`;
                
            } catch (error) {
                console.error('Yükleme hatası:', error);
                alert(`Dosya yüklenemedi: ${files[i].name}\nHata: ${error.message}`);
            }
        }

        // Upload tamamlandıktan sonra
        setTimeout(() => {
            uploadProgress.style.display = 'none';
            progressBar.style.width = '0%';
            progressText.textContent = '0%';
            fileInput.value = '';
            loadMusicList();  // Playlist'i güncelle
        }, 1000);
    };

    // Kayıt işlemleri
    let mediaRecorder;
    let audioChunks = [];
    let isRecording = false;
    let recordingTimer;
    let recordingTime = 0;

    const recordButton = document.getElementById('recordButton');
    const recordStatus = document.getElementById('recordStatus');
    const recordTime = document.getElementById('recordTime');

    recordButton.addEventListener('click', async () => {
        alert('⚠️ Ses kayıt özelliği geliştirme aşamasındadır.\nBir sonraki güncellemede eklenecektir.');
    });
>>>>>>> stable-power-audio
}); 