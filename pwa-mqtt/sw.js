const CACHE_NAME = 'mqtt-monitor-v1';
const urlsToCache = [
  '/',
  '/index.html',
  '/app.js',
  '/manifest.json',
  '/favicon.ico',
  '/icon-192.png',
  '/icon-512.png',
  'https://unpkg.com/mqtt@4.3.7/dist/mqtt.min.js'
];

// Installation du SW
self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => {
        return cache.addAll(urlsToCache);
      })
  );
});

// Activation du SW
self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(cacheNames => {
      return Promise.all(
        cacheNames.filter(cacheName => {
          return cacheName !== CACHE_NAME;
        }).map(cacheName => {
          return caches.delete(cacheName);
        })
      );
    })
  );
});

// Stratégie de cache: Network First, fallback to cache
self.addEventListener('fetch', event => {
  event.respondWith(
    fetch(event.request)
      .catch(() => {
        return caches.match(event.request);
      })
  );
});

// Gestion des notifications
self.addEventListener('push', event => {
  const data = event.data ? event.data.json() : { title: 'Notification', body: 'Nouvelle notification' };

  const options = {
    body: data.body,
    icon: 'icon-192.png',
    vibrate: [100, 50, 100],
    data: {
      dateOfArrival: Date.now(),
      primaryKey: 1
    }
  };

  event.waitUntil(
    self.registration.showNotification(data.title, options)
  );
});

// Pour communiquer avec le client (app.js)
self.addEventListener('message', event => {
  if (event.data.action === 'showNotification') {
    const notificationData = event.data.notification;
    self.registration.showNotification(notificationData.title, {
      body: notificationData.body,
      icon: 'icon-192.png'
    });
  }
});