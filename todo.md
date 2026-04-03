# TODO

- [x] TX: passer toutes les clés JSON historiques en majuscules partout
- [ ] TX et RX: Restructurer les messages JSONL de façon plus logique (surtout RX)
- [ ] TX et RX: améliorer la réactivité de détection
  - [ ] TX: réduire le temps de démarrage de TX :
      - [ ] supprimer le bootloader
      - [ ] vérifier le ΔT de l'AM312
      - [ ] optimiser les paramètres LoRa
- [ ] implémenter un système de chiffrement
      - [ ] pour l'appairage
      - [ ] et la confidentialité
- [ ] TX: réduire la taille du payload LoRa au strict nécessaire (finaliser le mode non-DEBUG)
- [ ] TX et RX: réaliser un PCB
- [ ] Simplifier la gestion des settings et particulièrement des secrets
      https://github.com/skills/introduction-to-secret-scanning/tree/main
- [ ] Mettre la doc à jour
- [ ] Remplacer NTFY par une solution 100% MQTT
- [ ] Implémenter une solution LoRaWAN, probablement via TTN

