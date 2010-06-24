"""
Module for handling Fekete points approximations.
"""

from numpy import empty
from numpy.linalg import solve

from gauss_lobatto_points import points

def get_x_phys(x_ref, a, b):
    return (a+b)/2. + x_ref*(b-a)/2.;

class Mesh1D(object):

    def __init__(self, points, orders):
        if not (len(points) == len(orders) + 1):
            raise Exception("points vs order mismatch")
        self._points = points
        self._orders = orders

    def iter_elems(self):
        for i in range(len(self._orders)):
            yield (self._points[i], self._points[i+1], self._orders[i])

class Function(object):
    """
    Represents a function on a mesh.

    The values are given in the Fekete points.
    """

    def __init__(self, obj, mesh=None):
        if not isinstance(mesh, Mesh1D):
            raise Exception("You need to specify a mesh.")
        self._mesh = mesh
        self._values = []
        for a, b, order in mesh.iter_elems():
            fekete_points = points[order]
            elem_values = []
            for p in fekete_points:
                p = get_x_phys(p, a, b)
                val = obj(p)
                elem_values.append(val)
            self._values.append(elem_values)

    def get_polynomial(self, values, a, b):
        """
        Returns the interpolating polynomial's coeffs.

        The len(values) specifies the order and we work in the element <a, b>
        """
        n = len(values)
        A = empty((n, n), dtype="double")
        y = empty((n,), dtype="double")
        x = points[n-1]
        assert len(x) == n
        for i in range(n):
            for j in range(n):
                A[i, j] = get_x_phys(x[i], a, b)**(n-j-1)
            y[i] = values[i]
        a = solve(A, y)
        return a

    def eval_polynomial(self, coeffs, x):
        r = 0
        n = len(coeffs)
        for i, a in enumerate(coeffs):
            r += a*x**(n-i-1)
        return r

    def __call__(self, x):
        for n, (a, b, order) in enumerate(self._mesh.iter_elems()):
            if b < x:
                continue
            # This can be made faster by using Lagrange interpolation
            # polynomials (no need to invert a matrix in order to get the
            # polynomial below). The results are however identical.
            coeffs = self.get_polynomial(self._values[n], a, b)
            return self.eval_polynomial(coeffs, x)

    def project_onto(self, mesh):
        # This is not a true projection, only some approximation:
        return Function(self, mesh)



def test1():
    m = Mesh1D((-5, -4, 3, 10), (1, 5, 1))

def test2():
    eps = 1e-12
    func = lambda x: x**2
    f = Function(func, Mesh1D((-5, -4, 3, 10), (2, 5, 2)))
    for x in [-5, -4.5, -4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3, 4, 5, 6, 7, 10]:
        assert abs(f(x) - func(x)) < eps

    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 2)))
    for x in [-5, -4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3, 4, 5, 6, 7, 10]:
        assert abs(f(x) - func(x)) < eps
    x = -4.9
    assert abs(f(x) - func(x)) > 0.08
    x = -4.5
    assert abs(f(x) - func(x)) > 0.24

    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 1)))
    for x in [-5, -4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3, 10]:
        assert abs(f(x) - func(x)) < eps
    x = -4.9
    assert abs(f(x) - func(x)) > 0.08
    x = -4.5
    assert abs(f(x) - func(x)) > 0.24
    x = 4
    assert abs(f(x) - func(x)) > 5.9
    x = 5
    assert abs(f(x) - func(x)) > 9.9
    x = 6
    assert abs(f(x) - func(x)) > 11.9
    x = 7
    assert abs(f(x) - func(x)) > 11.9
    x = 8
    assert abs(f(x) - func(x)) > 9.9
    x = 9
    assert abs(f(x) - func(x)) > 5.9

def test3():
    eps = 1e-12
    func = lambda x: x**2
    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 1)))
    for x in [-4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3]:
        assert abs(f(x) - func(x)) < eps

    func = lambda x: x**3
    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 1)))
    for x in [-4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3]:
        assert abs(f(x) - func(x)) < eps

    func = lambda x: x**4
    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 1)))
    for x in [-4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3]:
        assert abs(f(x) - func(x)) < eps

    func = lambda x: x**5
    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 1)))
    for x in [-4, -3, -2, -1, 0, 0.01, 1e-5, 1, 2, 3]:
        assert abs(f(x) - func(x)) < eps

    func = lambda x: x**6
    f = Function(func, Mesh1D((-5, -4, 3, 10), (1, 5, 1)))
    x = -1
    assert abs(f(x) - func(x)) > 61.9
    x = 0
    assert abs(f(x) - func(x)) > 61.9
    x = 1
    assert abs(f(x) - func(x)) > 61.6
    x = 2
    assert abs(f(x) - func(x)) > 28.9

def test4():
    eps = 1e-12
    func = lambda x: x**2
    orig_mesh = Mesh1D((-5, -4, 3, 10), (1, 5, 1))
    mesh1     = Mesh1D((-5, -4, 3, 10), (1, 1, 1))
    f = Function(func, orig_mesh)
    g = f.project_onto(mesh1)

def main():
    test1()
    test2()
    test3()
    test4()

if __name__ == "__main__":
    main()
