import numpy as np
from scipy.optimize import fsolve
import matplotlib.pyplot as plt


l1 = np.linspace(30,50,20) # Fixed arm of the rotating member
l2 = np.linspace(30,50,20) # Free arm attached to the thing
l3 = np.linspace(15,50,20)
l4 = 35 #Approximate height of the hinge

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


plate_limit_theta = np.array([-25,25])
plate_limit_gamma = 90 - plate_limit_theta


gamma = np.linspace(np.deg2rad(plate_limit_gamma[0]), np.deg2rad(plate_limit_gamma[1]), 3, endpoint=True)

phis = np.zeros_like(l1_mesh)
for i in range(len(l1)):
    for j in range(len(l2)):
        for k in range(len(l3)):
            d = 0
            phi = np.pi/2
            for g in gamma:
                phi_new = quadsolve_phi(l1[i], l2[j], l3[k], l4, g)
                if np.isnan(phi_new):
                    phi = phi_new
                    d = float("nan")
                    break
                d_new = phi_new - np.pi/2
                if np.abs(d_new) > d:
                    phi = phi_new
                    d = np.abs(d_new)


            phis[i][j][k] = phi





fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
scatter = ax.scatter(l1_mesh, l2_mesh, l3_mesh, c=np.sin(phis), cmap='PRGn')
fig.colorbar(scatter, ax=ax)
plt.show()