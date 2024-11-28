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
            })
            .catch(console.error);
    }

    // İlk yükleme
    loadMusicList();
    updateStatus();
    
    // Periyodik güncelleme
    setInterval(updateStatus, 1000);
    setInterval(loadMusicList, 5000);  // MP3 listesini her 5 saniyede bir güncelle
}); 