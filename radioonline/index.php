<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Online Rádio Přehrávač</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/css/bootstrap.min.css">
    <style>
        body {
            display: flex;
            background-color: #121212;
            color: #e0e0e0;
            font-family: Arial, sans-serif;
        }
        #sidebar {
            width: 25;
            height: 100vh;
            background: #1e1e1e;
            padding: 15px;
            overflow-y: auto;
            border-right: 1px solid #333;
        }
        #content {
            flex: 1;
            padding: 15px;
        }
        .list-group-item {
            background-color: #2e2e2e;
            color: #e0e0e0;
            border: 1px solid #444;
        }
        .list-group-item:hover {
            background-color: #444;
        }
        .btn-sm {
            background: none;
            border: none;
            font-size: 1.2em;
            cursor: pointer;
        }
        .btn-sm:hover {
            color: #ffca28;
        }
        #player {
            background-color: #1e1e1e;
            border-radius: 10px;
            padding: 10px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        #player-controls {
            display: flex;
            align-items: center;
            gap: 15px;
        }
        #songInfo img {
            border-radius: 10px;
        }
    </style>
</head>
<body>

<div id="sidebar">
    <input type="text" id="search" class="form-control mb-3" placeholder="Hledat rádio">
    <ul id="radioList" class="list-group"></ul>
</div>

<div id="content">
    <h3 id="radioName">Vyberte rádio</h3>
    <div id="player">
        <audio id="audioPlayer" style="flex-grow: 1;">
            <source id="audioSource" src="" type="audio/mpeg">
        </audio>
        <div id="player-controls">
            <button id="playPauseBtn" class="btn btn-secondary">▶</button>
            <input type="range" id="volumeSlider" min="0" max="1" step="0.1">
        </div>
    </div>
    <div id="songInfo" class="mt-3">
        <h5>Co hraje právě teď:</h5>
        <p><strong id="songName">-</strong> od <strong id="songArtist">-</strong></p>
        <img id="songImage" src="" alt="" style="max-width: 200px; max-height: 200px;">
    </div>
</div>

<script>
    const radioListUrl = 'list.json';
    const radioStreamBaseUrl = 'https://radia.cz/api/v1/radio/';

    const searchInput = document.getElementById('search');
    const radioListElement = document.getElementById('radioList');
    const radioNameElement = document.getElementById('radioName');
    const audioPlayer = document.getElementById('audioPlayer');
    const audioSource = document.getElementById('audioSource');
    const playPauseBtn = document.getElementById('playPauseBtn');
    const volumeSlider = document.getElementById('volumeSlider');
    const songNameElement = document.getElementById('songName');
    const songArtistElement = document.getElementById('songArtist');
    const songImageElement = document.getElementById('songImage');

    let radios = [];
    let selectedRadio = null;
    let favorites = [];

    // Load favorites from cookies
    const favoritesCookie = document.cookie.split('; ').find(row => row.startsWith('favorites='));
    if (favoritesCookie) {
        favorites = JSON.parse(decodeURIComponent(favoritesCookie.split('=')[1]));
    }

    // Load saved volume from cookies
    const volumeCookie = document.cookie.split('; ').find(row => row.startsWith('volume='));
    const savedVolume = volumeCookie ? parseFloat(volumeCookie.split('=')[1]) : 0.5;
    audioPlayer.volume = savedVolume;
    volumeSlider.value = savedVolume;

    // Save volume on change
    volumeSlider.addEventListener('input', (e) => {
        const volume = e.target.value;
        audioPlayer.volume = volume;
        document.cookie = `volume=${volume}; path=/;`;
    });

    // Load radios from JSON
    fetch(radioListUrl)
        .then(response => response.json())
        .then(data => {
            radios = data;
            renderRadios(radios);

            // Load selected radio from cookies if available
            const cookieRadioSlug = document.cookie.split('; ').find(row => row.startsWith('selectedRadio='));
            if (cookieRadioSlug) {
                const slug = cookieRadioSlug.split('=')[1];
                const savedRadio = radios.find(radio => radio.slug === slug);
                if (savedRadio) selectRadio(savedRadio);
            }
        });

    function renderRadios(radios) {
        radioListElement.innerHTML = '';

        // Render favorites first
        favorites.forEach(favSlug => {
            const radio = radios.find(r => r.slug === favSlug);
            if (radio) {
                const li = createRadioListItem(radio, true);
                radioListElement.appendChild(li);
            }
        });

        // Render other radios
        radios.forEach(radio => {
            if (!favorites.includes(radio.slug)) {
                const li = createRadioListItem(radio, false);
                radioListElement.appendChild(li);
            }
        });
    }

    function createRadioListItem(radio, isFavorite) {
        const li = document.createElement('li');
        li.className = 'list-group-item d-flex justify-content-between align-items-center';
        li.textContent = radio.name;
        li.dataset.slug = radio.slug;
        li.addEventListener('click', () => selectRadio(radio));

        const favButton = document.createElement('button');
        favButton.className = 'btn btn-sm';
        favButton.textContent = isFavorite ? '★' : '☆';
        favButton.style.color = isFavorite ? 'gold' : 'gray';
        favButton.addEventListener('click', (e) => {
            e.stopPropagation(); // Prevent triggering the selectRadio
            toggleFavorite(radio.slug);
        });

        li.appendChild(favButton);
        return li;
    }

    function toggleFavorite(slug) {
        if (favorites.includes(slug)) {
            favorites = favorites.filter(fav => fav !== slug);
        } else {
            favorites.push(slug);
        }
        document.cookie = `favorites=${encodeURIComponent(JSON.stringify(favorites))}; path=/;`;
        renderRadios(radios);
    }

    function selectRadio(radio) {
        selectedRadio = radio;
        radioNameElement.textContent = radio.name;
        fetch(`${radioStreamBaseUrl}${radio.slug}/streams.json`)
            .then(response => response.json())
            .then(streams => {
                const streamUrl = streams[0]; // Use the first stream URL
                audioSource.src = streamUrl;
                audioPlayer.load();
                audioPlayer.play();
                document.cookie = `selectedRadio=${radio.slug}; path=/;`;
                fetchNowPlaying();
            });
    }

    function fetchNowPlaying() {
        if (!selectedRadio) return;
        fetch(`${radioStreamBaseUrl}${selectedRadio.slug}/songs/now.json`)
            .then(response => response.json())
            .then(song => {
                songNameElement.textContent = song.song || '-';
                songArtistElement.textContent = song.interpret || '-';
                songImageElement.src = song.image || '';
            });
    }

    playPauseBtn.addEventListener('click', () => {
        if (audioPlayer.paused) {
            audioPlayer.play();
            playPauseBtn.textContent = '⏸';
        } else {
            audioPlayer.pause();
            playPauseBtn.textContent = '▶';
        }
    });

    searchInput.addEventListener('input', () => {
        const query = searchInput.value.toLowerCase();
        const filteredRadios = radios.filter(radio => radio.name.toLowerCase().includes(query));
        renderRadios(filteredRadios);
    });

    setInterval(fetchNowPlaying, 5000); // Refresh now playing every 5 seconds
</script>

<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>
