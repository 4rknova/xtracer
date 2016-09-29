camera = {
	default = {
        type = thin-lens

		fov = 60

		position = {
			x = 450
			y = 400
			z = -1000
		}

		target = {
		    x = 0
            y = 0
			z = 0
		}

		up = {
			y = 1
		}
	}
}

geometry = {
	sphere10 = {
		type = sphere
		position = {
			x = 325
			y = 175
			z = 850
		}
		radius = 175
	}

	sphere11 = {
		type = sphere
		position = {
			x = -50
			y = 175
			z = 500
		}
		radius = 175
	}
	sphere12 = {
		type = sphere
		position = {
			x = -350
			y = 175
			z = 50
		}
		radius = 175
	}

	sphere2 = {
		type = sphere
		position = {
			x = 300
			y = 175
			z = -80
		}
		radius = 175
	}

	plane = {
		type = plane
		normal = {
			y = 1
		}

		v_scale = 0.003
		u_scale = 0.003
	}
}

light = {
	front = {
		type = sphere

        radius = 25

		position = {
			y = 900
			z = -800
		}

		intensity = {
			r = 1
			g = 1
			b = 1
		}
	}

	cat = {
		type = sphere

        radius = 25

		position = {
			x = -1000
			y = 1100
			z = 1800
		}

		intensity = {
			r = 1
			g = 1
			b = 1
		}
	}
}

material = {
	floor = {
		type = phong

		k_exponent = 255

		diffuse = {
			r = .3
			g = .3
			b = .3
		}

		specular = {
			r = .7
			g = .7
			b = .7
		}

		reflectance = 0.7
	}

	ball1 = {
		type = blinn_phong

		k_exponent = 60

		diffuse = {
            r = 0.5
		}

		specular = {
			r = 0.5
			g = 0.5
			b = 0.5
		}

		reflectance = 0.5
	}

	ball2 = {
		type = blinn_phong

		k_exponent = 560

		diffuse = {
			g = 0.5
		}

		specular = {
			r = 0.5
			g = 0.5
			b = 0.5
		}

		reflectance = 0.5

	}

	ball3 = {
		type = blinn_phong

		k_exponent = 60

		diffuse = {
			b = 0.5
		}

		specular = {
			r = 0.5
			g = 0.5
			b = 0.5
		}

		reflectance = 0.5

	}
}

texture = {
	floor = {
		source = <texdir>/wood01.ppm
	}
}

object = {
	sphere11 = {
		geometry = sphere11
		material = ball1
	}
	sphere12 = {
		geometry = sphere12
		material = ball2
	}
	sphere10 = {
		geometry = sphere10
		material = ball3
	}
	floor = {
		geometry = plane
		material = floor
		texture  = floor
	}
}
