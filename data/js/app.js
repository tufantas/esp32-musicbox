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
                        console.log('Clicking on:', file);
                        fetch('/api/play', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                                'Accept': 'application/json'
                            },
                            body: JSON.stringify({ 
                                file: file.startsWith('/') ? file : '/' + file 
                            })
                        })
                        .then(response => {
                            if (!response.ok) {
                                throw new Error(`HTTP error! status: ${response.status}`);
                            }
                            console.log('Play request sent successfully');
                        })
                        .catch(error => {
                            console.error('Error:', error);
                            alert(`Failed to play: ${file}`);
                        });
                    };
                    musicList.appendChild(li);
                });
            })
            .catch(console.error);
    }

    // İlk durum güncellemesi ve periyodik güncelleme
    function updateStatus() {
        fetch('/api/status')
            .then(response => {
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                return response.json();
            })
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
                // Volume güncellemesini sadece değer değişmişse yap
                if (data.volume && Math.abs(volumeSlider.value - data.volume) > 1) {
                    volumeSlider.value = data.volume;
                    document.getElementById('volumeValue').textContent = data.volume + '%';
                }
            })
            .catch(error => {
                console.error('Status update error:', error);
            });
    }

    // İlk yükleme
    loadMusicList();
    updateStatus();
    
    // Periyodik güncelleme
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
            alert('Please select a file to upload');
            return;
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
                    throw new Error(`HTTP error! status: ${response.status}`);
                }

                progressBar.style.width = `${((i + 1) / files.length) * 100}%`;
                progressText.textContent = `${Math.round(((i + 1) / files.length) * 100)}%`;
                
            } catch (error) {
                console.error('Error:', error);
                alert(`Failed to upload ${files[i].name}`);
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
}); 