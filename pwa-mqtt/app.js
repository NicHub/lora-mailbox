document.addEventListener('DOMContentLoaded', () => {
  const connectionStatus = document.getElementById('connection-status');
  const log = document.getElementById('log');
  const notificationCheckbox = document.getElementById('notification-permission');
  const heartbeatCounter = document.getElementById('heartbeat-counter').querySelector('.value');
  const noHeartbeatCounter = document.getElementById('no-heartbeat-counter').querySelector('.value');
  let client = null;
  let isConnected = false; // Drapeau pour éviter les connexions multiples
  let swRegistration = null; // Pour stocker l'enregistrement du service worker

  // Compteurs
  let heartbeatCount = 0;
  let noHeartbeatCount = 0;

  // Enregistrement du Service Worker
  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.getRegistrations().then(registrations => {
      if (registrations.length === 0) {
        navigator.serviceWorker.register('/sw.js')
          .then(reg => {
            console.log('Service Worker enregistré', reg);
            swRegistration = reg;
          })
          .catch(err => console.error('Erreur d\'enregistrement du SW:', err));
      } else {
        console.log('Service Worker déjà enregistré');
        swRegistration = registrations[0]; // Utiliser l'enregistrement existant
      }
    });
  }

  // Initialiser l'état de la case à cocher selon la permission actuelle
  if (Notification.permission === 'granted') {
    notificationCheckbox.checked = true;
  }

  // Gestion de la case à cocher pour les notifications
  notificationCheckbox.addEventListener('change', () => {
    if (notificationCheckbox.checked) {
      Notification.requestPermission().then(permission => {
        if (permission === 'granted') {
          if (!isConnected) {
            connectMQTT(); // Connecter après autorisation, seulement si pas déjà connecté
          }
        } else {
          // Si la permission est refusée, décocher la case
          notificationCheckbox.checked = false;
          addLogMessage('Permission de notification refusée', 'disconnected');
        }
      });
    } else {
      // Désactivation des notifications (ne peut pas révoquer la permission,
      // mais peut arrêter d'en envoyer)
      addLogMessage('Notifications désactivées par l\'utilisateur', 'message');
    }
  });

  function updateCounters() {
    heartbeatCounter.textContent = heartbeatCount;
    noHeartbeatCounter.textContent = noHeartbeatCount;
  }

  function addLogMessage(message, type = '') {
    const messageDiv = document.createElement('div');
    messageDiv.textContent = `${new Date().toLocaleTimeString()}: ${message}`;
    messageDiv.className = `status ${type}`;
    log.prepend(messageDiv);

    // Limiter le nombre de messages dans le log (optionnel)
    if (log.children.length > 30) {
      log.removeChild(log.lastChild);
    }
  }

  function showNotification(title, body) {
    // Vérifier si les notifications sont activées (case cochée)
    if (!notificationCheckbox.checked) {
      addLogMessage('Notification supprimée (désactivée par l\'utilisateur)', 'message');
      return;
    }

    if (!('Notification' in window)) {
      addLogMessage('Ce navigateur ne prend pas en charge les notifications', 'disconnected');
      return;
    }

    if (Notification.permission === 'granted') {
      // Utiliser le service worker pour afficher la notification
      if (swRegistration) {
        swRegistration.showNotification(title, {
          body: body,
          icon: 'icon-192.png',
          vibrate: [100, 50, 100]
        }).catch(err => {
          addLogMessage(`Erreur de notification: ${err.message}`, 'disconnected');
        });
      } else {
        // Fallback - cette approche pourrait générer l'erreur dans certains contextes
        try {
          navigator.serviceWorker.ready.then(registration => {
            registration.showNotification(title, {
              body: body,
              icon: 'icon-192.png',
              vibrate: [100, 50, 100]
            });
          });
        } catch (e) {
          addLogMessage(`Erreur de notification: ${e.message}`, 'disconnected');
        }
      }
    }
  }

  function connectMQTT() {
    // Vérifier si déjà connecté
    if (isConnected) {
      console.log("Déjà connecté au broker MQTT");
      return;
    }

    // Marquer comme en cours de connexion
    isConnected = true;

    // Connexion au broker MQTT
    const clientId = 'mqttjs_' + Math.random().toString(16).substr(2, 8);
    const host = 'wss://test.mosquitto.org:8081';

    const options = {
      keepalive: 60,
      clientId: clientId,
      protocolId: 'MQTT',
      protocolVersion: 4,
      clean: true,
      reconnectPeriod: 1000,
      connectTimeout: 30 * 1000
    };

    // Fermer toute connexion existante
    if (client) {
      try {
        client.end(true);
      } catch (e) {
        console.error("Erreur lors de la fermeture de la connexion existante", e);
      }
    }

    client = mqtt.connect(host, options);

    client.on('connect', () => {
      connectionStatus.textContent = 'Connecté au broker MQTT';
      connectionStatus.className = 'status connected';
      addLogMessage('Connexion établie', 'connected');

      // S'abonner au topic
      client.subscribe('mailboxtest', err => {
        if (!err) {
          addLogMessage('Abonné au topic mailboxtest', 'connected');
        }
      });
    });

    client.on('message', (topic, message) => {
      const messageStr = message.toString();
      addLogMessage(`Message reçu: ${messageStr}`, 'message');

      try {
        const data = JSON.parse(messageStr);
        // Vérifier si le champ HEARTBEAT existe
        if ('HEARTBEAT' in data) {
          heartbeatCount++;
        } else {
          noHeartbeatCount++;
          showNotification(
            'Alerte MQTT',
            'Message sans HEARTBEAT détecté: ' + messageStr
          );
        }
        // Mettre à jour les compteurs
        updateCounters();
      } catch (e) {
        addLogMessage(`Erreur de parsing JSON: ${e.message}`, 'disconnected');
      }
    });

    client.on('error', err => {
      addLogMessage(`Erreur MQTT: ${err}`, 'disconnected');
      connectionStatus.textContent = 'Erreur de connexion';
      connectionStatus.className = 'status disconnected';
      isConnected = false; // Réinitialiser le drapeau de connexion
    });

    client.on('offline', () => {
      addLogMessage('Client MQTT déconnecté', 'disconnected');
      connectionStatus.textContent = 'Déconnecté';
      connectionStatus.className = 'status disconnected';
      isConnected = false; // Réinitialiser le drapeau de connexion
    });
  }

  // Initialiser les compteurs
  updateCounters();

  // Vérifier si les notifications sont déjà autorisées
  if (Notification.permission === 'granted') {
    connectMQTT();
  }
});