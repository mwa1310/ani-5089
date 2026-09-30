## Résultats

*Poses utilisées*

- Parent : rotation 90° autour de +y, translation (1, 2, 3)
- Enfant : rotation 90° autour de +x, translation (0, 1, 0), exprimée dans le repère du parent
- Point : (0.5, -1, 2), exprimé dans le repère de l'enfant

## Comparaison des deux méthodes*

- Composer puis appliquer : 0.0000 1.0000 2.5000
- Appliquer enfant puis parent : 0.0000 1.0000 2.5000
- Ecart (norme) : 0.00000055


## Explication

Les deux méthodes donnent le même point, aux erreurs d'arrondi en flottant près (5,5×10⁻⁷, négligeable, le bruit habituel d'un calcul en `float`). C'est exactement ce que doit garantir la composition de deux poses :

*compose(parent, enfant) appliquée une seule fois au point local équivaut à appliquer l'enfant au point, puis appliquer le parent au résultat, étape par étape.*


La fonction `compose` construit cette équivalence directement dans ses deux composantes :
- l'orientation composée est le produit des deux quaternions, parent d'abord : `R_monde = R_parent * R_enfant` ;
- la position composée est la position de l'enfant, tournée par l'orientation du parent, puis décalée par la position du parent : `t_monde = R_parent * t_enfant + t_parent`.

## Pourquoi c'est important

C'est cette équivalence qui permet l'exemple du cours : faire tourner l'épaule et voir la main suivre sans qu'on ait rien à lui redire. 

Une chaîne épaule -> coude -> main peut être composée une seule fois en une pose unique « main dans le monde », plutôt que réappliquée maillon par maillon à chaque point de la main avec la garantie que le résultat est identique dans les deux cas.