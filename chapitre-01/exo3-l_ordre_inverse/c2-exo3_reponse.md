**Résultats**

-- Cas général  
Pose : rotation 90° autour de +y, translation t = (1, 2, 3) ; point p = (0, 0, -1)

rotation puis translation : 0.0000 2.0000 3.0000
translation puis rotation : 2.0000 2.0000 -1.0000  - DIFFERENTS

-- Cas particulier 
Pose : rotation 90° autour de +y, translation t = (0, 5, 0) ; point p = (1, 2, 3)

rotation puis translation : 3.0000 7.0000 -1.0000
translation puis rotation : 3.0000 7.0000 -1.0000  - IDENTIQUES

Même pose, point totalement différent 
p = (-7, 100, 0.5)

rotation puis translation : 0.5000 105.0000 7.0000
translation puis rotation : 0.5000 105.0000 7.0000  - IDENTIQUES

-- Pourquoi les deux coïncident dans le second cas

Les deux ordres s'écrivent :

rotation puis translation :  R·p + t
translation puis rotation :  R·(p + t) = R·p + R·t

Ils sont égaux exactement quand `t = R·t`, c'est-à-dire quand la translation est un vecteur invariant par la rotation. Le point `p` disparaît entièrement de cette condition : l'égalité ne dépend jamais du point choisi, seulement de la pose. C'est pourquoi le troisième test, avec un point pris très loin et très différent, coïncide encore lui aussi.

Un vecteur est invariant par une rotation quand il est aligné sur l'axe de rotation : tourner autour de +y ne change rien à une composante purement en y, de la même façon que l'aiguille d'une horloge ne déplace pas son propre axe. C'est le seul cas non trivial où l'ordre n'a pas d'importance; les deux autres cas, triviaux, étant une rotation nulle (identité) ou une translation nulle.

-- Ce que cela dit sur l'ordre en général

L'ordre n'est presque jamais indifférent. Le seul moment où il l'est révèle exactement pourquoi il compte d'habitude : faire tourner un point *après* l'avoir déplacé fait aussi tourner le déplacement lui-même, sauf quand ce déplacement pointe déjà dans la direction que la rotation laisse tranquille. C'est cette dépendance à l'ordre qui impose, dans une pose, la convention fixée une fois pour toutes : on tourne d'abord, on translate ensuite; jamais l'inverse, sauf à vouloir délibérément un résultat différent.