import numpy as np
from scipy.optimize import fsolve
import matplotlib.pyplot as plt

RES = 30
l1 = np.linspace(30,100,RES) # Fixed arm of the rotating member
l2 = np.linspace(30,100,RES) # Free arm attached to the thing
l3 = np.linspace(30,100,RES) # Hinge X position
l4 = 35 #Approximate height of the hinge

def quadsolve_gen(l1, l2, l3, l4, theta):
    """
    Given an output rod angle theta that L3 makes with L4, solves for the angles alpha, beta, and gamma
    
    alpha: l4 l1
    
    beta: l1 l2
    
    gamma: l2 l3
    
    theta: l3 l4 (given)
    """
    d1 = np.sqrt(l3**2 + l4**2 - 2*l3*l4*np.cos(theta))
    beta = np.arccos( ( d1**2 - (l1**2 + l2**2)) / (-2 * l1 * l2 ) )
    alpha = np.arcsin( (l3/d1) * np.sin(theta)) + np.arcsin( (l2/d1) * np.sin(beta))
    gamma = np.arcsin( (l4/d1) * np.sin(theta)) + np.arcsin( (l1/d1) * np.sin(beta))
    return (beta, alpha, gamma)
    
        
    

    
    
def quadsolve_phi(l1, l2, l3, l4, gamma):
    # Cosine law
    d1_squared = l3**2 + l4**2 - 2*l3*l4*np.cos(gamma)
    # if squared < 0, past limit range of zero angle
    if d1_squared < 0 :
        return float("nan")

    cos_phi = (d1_squared - l1**2 - l2**2) / (-1*l1*l2)    
    # if cos_phi not between (-1,1), past 180deg limit angle
    if abs(cos_phi) > 1:
        return float("nan")
    phi = np.arccos(cos_phi)
    return phi
    
l1_mesh,l2_mesh, l3_mesh = np.meshgrid(l1, l2, l3)

plate_limit_angle = np.array([-25,25])
plate_limit_theta = 90 - plate_limit_angle


theta = np.linspace(np.deg2rad(plate_limit_theta[0]), np.deg2rad(plate_limit_theta[1]), RES, endpoint=True)

advantage = np.zeros_like(l1_mesh)
for i in range(len(l1)):
    for j in range(len(l2)):
        for k in range(len(l3)):
            adv = np.inf #Best case scenario is infinite advantage
            for t in theta:
                alpha, beta, gamma = quadsolve_gen(l1[i], l2[j], l3[k], l4, t)
                adv_new = (l3[k]*np.sin(gamma))/(l1[i]*np.sin(beta))
                if adv > adv_new:
                    adv = adv_new
                if np.isnan(adv_new) or np.isnan(adv):
                    adv = np.nan


            advantage[i][j][k] = adv


print(advantage)

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
scatter = ax.scatter(l1_mesh, l2_mesh, l3_mesh, c=advantage, cmap='YlGn')
fig.colorbar(scatter, ax=ax)
plt.show()