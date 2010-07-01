"""
Module for handling Fekete points approximations.
"""

from math import pi, sin, log, sqrt

from numpy import empty, arange, array, ndarray
from numpy.linalg import solve

from scipy.integrate import quadrature

from gauss_lobatto_points import points

def get_x_phys(x_ref, a, b):
    return (a+b)/2. + x_ref*(b-a)/2.;

def generate_candidates(a, b, order):
    def cand(divisions, orders):
        if len(divisions) == 0:
            assert len(orders) == 1
            return Mesh1D((a, b), (order + orders[0],))
        elif len(divisions) == 1:
            assert len(orders) == 2
            return Mesh1D((a, get_x_phys(divisions[0], a, b), b),
                    (order + orders[0], order + orders[1]))
        else:
            raise NotImplementedError()
    cands = [
            cand([], [0]),
            cand([], [1]),
            cand([], [2]),
            cand([0.5], [0, 0]),
            cand([0.5], [1, 0]),
            cand([0.5], [0, 1]),
            cand([0.5], [1, 1]),
            ]
    if order > 1:
        cands.extend([
            cand([0.5], [-1, 0]),
            cand([0.5], [0, -1]),
            cand([0.5], [-1, -1]),
            ])
    return cands

class Mesh1D(object):

    def __init__(self, points, orders):
        if not (len(points) == len(orders) + 1):
            raise Exception("points vs order mismatch")
        self._points = tuple(points)
        self._orders = tuple(orders)

    def iter_elems(self):
        for i in range(len(self._orders)):
            yield (self._points[i], self._points[i+1], self._orders[i])

    def plot(self, call_show=True):
        try:
            from jsplot import plot, show
        except ImportError:
            from pylab import plot, show
        odd = False
        for a, b, order in self.iter_elems():
            fekete_points = points[order]
            fekete_points = [get_x_phys(x, a, b) for x in fekete_points]
            if odd:
                format = "y-"
            else:
                format = "k-"
            odd = not odd
            plot([a, a, b, b], [0, order, order, 0], format, lw=2)
        if call_show:
            show()

    def element_at_point(self, x):
        """
        Returns the element id at a point "x".

        """
        for n, (a, b, order) in enumerate(self.iter_elems()):
            if b < x:
                continue
            return n

    def get_element_by_id(self, id):
        return list(self.iter_elems())[id]

    def get_node_id_by_coord(self, x):
        eps = 1e-10
        for i, node in enumerate(self._points):
            if abs(node-x) < eps:
                return i
        raise ValueError("Node not found")

    def restrict_to_elements(self, n1, n2):
        """
        Returns the submesh of elements [n1, n2] (in the slice notation).
        """
        return Mesh1D(self._points[n1:n2+1], self._orders[n1:n2])

    def restrict_to_interval(self, A, B):
        assert B > A
        n1 = self.element_at_point(A)
        n2 = self.element_at_point(B)
        points = []
        orders = []

        # first element:
        a, b, order = self.get_element_by_id(n1)
        eps = 1e-12
        if abs(b-A) < eps:
            pass
        else:
            if abs(a-A) < eps:
                pass
            elif a < A:
                a = A
            else:
                raise NotImplementedError()
            points.append(a)
            orders.append(order)

        #middle elements
        for n in range(n1+1, n2):
            a, b, order = self.get_element_by_id(n)
            points.append(a)
            orders.append(order)

        # last element:
        a, b, order = self.get_element_by_id(n2)
        eps = 1e-12
        if abs(a-A) < eps:
            pass
        elif a < A:
            a = A
        if abs(b-B) < eps:
            pass
        elif B < b:
            b = B
        else:
            raise NotImplementedError()
        if len(points) == 0 or not (abs(points[-1] - a) < eps):
            points.append(a)
            orders.append(order)
        points.append(b)

        return Mesh1D(points, orders)

    def union(self, o):
        eps = 1e-12
        p1 = self._points
        p2 = o._points
        p = list(p1)
        p.extend(p2)
        p.sort()
        points = [p[0]]
        for point in p[1:]:
            if abs(points[-1] - point) < eps:
                continue
            points.append(point)
        # points now contains the sorted list of union points
        orders = []
        for n, p in enumerate(points[1:]):
            p1 = points[n]
            p2 = p
            mid = (p1+p2)/2.
            o1 = self._orders[self.element_at_point(mid)]
            o2 = o._orders[o.element_at_point(mid)]
            orders.append(max(o1, o2))

        return Mesh1D(points, orders)

    def __eq__(self, o):
        eps = 1e-12
        if isinstance(o, Mesh1D):
            if self._orders == o._orders:
                d = array(self._points) - array(o._points)
                if (abs(d) < eps).all():
                    return True
        return False

    def __ne__(self, o):
        return not self.__eq__(o)

    def use_candidate(self, cand):
        n1 = self.get_node_id_by_coord(cand._points[0])
        n2 = self.get_node_id_by_coord(cand._points[-1])
        points = self._points[:n1] + cand._points + self._points[n2+1:]
        orders = self._orders[:n1] + cand._orders + self._orders[n2:]
        return Mesh1D(points, orders)

class Function(object):
    """
    Represents a function on a mesh.

    The values are given in the Fekete points.
    """

    def __init__(self, obj, mesh=None):
        if not isinstance(mesh, Mesh1D):
            raise Exception("You need to specify a mesh.")
        self._mesh = mesh
        if isinstance(obj, (tuple, list, ndarray)):
            self._values = obj
        else:
            self._values = []
            for a, b, order in mesh.iter_elems():
                fekete_points = points[order]
                elem_values = []
                # Note: this is not a projection (it only evaluates obj in
                # fekete points), so the result is not the best
                # approximation possible:
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

    def restrict_to_interval(self, A, B):
        """
        Returns the same function, with the mesh (domain) restricted to the
        interval (A, B).
        """
        m = self._mesh.restrict_to_interval(A, B)
        return Function(self, m)


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

    def plot(self, call_show=True):
        try:
            from jsplot import plot, show
        except ImportError:
            from pylab import plot, show
        odd = False
        for n, (a, b, order) in enumerate(self._mesh.iter_elems()):
            fekete_points = points[order]
            vals = self._values[n]
            assert len(vals) == len(fekete_points)
            fekete_points = [get_x_phys(x, a, b) for x in fekete_points]
            x = arange(a, b, 0.1)
            y = [self(_x) for _x in x]
            if odd:
                format = "g-"
            else:
                format = "r-"
            odd = not odd
            plot(x, y, format)
            plot(fekete_points, vals, "ko")
        if call_show:
            show()

    def __eq__(self, o):
        eps = 1e-12
        if isinstance(o, Function):
            for a, b, order in self._mesh.iter_elems():
                fekete_points = points[order]
                fekete_points = [get_x_phys(x, a, b) for x in fekete_points]
                for p in fekete_points:
                    if abs(self(p) - o(p)) > eps:
                        return False
            for a, b, order in o._mesh.iter_elems():
                fekete_points = points[order]
                fekete_points = [get_x_phys(x, a, b) for x in fekete_points]
                for p in fekete_points:
                    if abs(self(p) - o(p)) > eps:
                        return False
            return True
        else:
            return False

    def __ne__(self, o):
        return not self.__eq__(o)

    def __add__(self, o):
        if self._mesh == o._mesh:
            values = array(self._values) + array(o._values)
            return Function(values, self._mesh)
        else:
            union_mesh = self._mesh.union(o._mesh)
            return self.project_onto(union_mesh) + o.project_onto(union_mesh)

    def __sub__(self, o):
        return self + (-o)

    def __neg__(self):
        values = [-array(x) for x in self._values]
        return Function(values, self._mesh)


    def get_mesh_adapt(self, max_order=12):
        return self._mesh

    def l2_norm(self):
        """
        Returns the L2 norm of the function.
        """
        i = 0
        def f(x):
            return [self(_)**2 for _ in x]
        for a, b, order in self._mesh.iter_elems():
            val, err = quadrature(f, a, b)
            i += val
        return i

    def dofs(self):
        """
        Returns the number of DOFs needed to represent the function.
        """
        n = 1
        for a, b, order in self._mesh.iter_elems():
            n += order
        return n

    def get_candidates_with_errors(self, f):
        """
        Returns a sorted list of all candidates and their errors.

        The best candidate is first, the worst candidate is last.

        The "f" is the reference function which we want to approximate using
        "self".
        """
        cand_with_errors = []
        for a, b, order in self._mesh.iter_elems():
            cands = generate_candidates(a, b, order)
            print "-"*40
            print a, b, order
            for m in cands:
                orig = self.restrict_to_interval(a, b)
                cand = Function(f, m)
                f2 = f.restrict_to_interval(a, b)
                dof_cand = cand.dofs()
                err_cand = (f2 - cand).l2_norm()
                dof_orig = orig.dofs()
                err_orig = (f2 - orig).l2_norm()
                if dof_cand == dof_orig:
                    if err_cand < err_orig:
                        # if this happens, it means that we can get better
                        # approximation with the same DOFs, so we definitely take
                        # this candidate:
                        crit = -1e10
                    else:
                        crit = 1e10 # forget this candidate
                elif dof_cand > dof_orig:
                    # if DOF rises, we take the candidate that goes the steepest in
                    # the log/sqrt error/DOFs convergence graph
                    # we want 'crit' as negative as possible:
                    crit = (log(err_cand) - log(err_orig)) / \
                            sqrt(dof_cand - dof_orig)
                else:
                    raise NotImplementedError("Derefinement not implemented yet.")
                cand_with_errors.append((m, crit))
        cand_with_errors.sort(key=lambda x: x[1])
        return cand_with_errors


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
    h = Function(func, mesh1)
    assert g == Function(func, mesh1)
    assert h == h.project_onto(orig_mesh)

def test5():
    eps = 1e-12
    func = lambda x: x**2
    mesh1 = Mesh1D((-5, -4, 3, 10), (2, 5, 2))
    mesh2 = Mesh1D((-5, -4, 3, 10), (2, 2, 2))
    mesh3 = Mesh1D((-5, -4, 3, 10), (2, 2, 1))
    mesh4 = Mesh1D((-5, 10), (2,))
    mesh5 = Mesh1D((-5, 10), (3,))
    mesh6 = Mesh1D((-5, 10), (1,))
    f = Function(func, mesh1)
    g = Function(func, mesh2)
    h = Function(func, mesh3)
    l = Function(func, mesh4)

    assert f == g
    assert g == f
    assert f == l
    assert g == l
    assert f != h
    assert h != f
    assert g != h
    assert h != g

    assert f == Function(lambda x: x**2, mesh1)
    assert f != Function(lambda x: x**3, mesh1)
    assert f == Function(lambda x: x**2, mesh2)
    assert f == Function(lambda x: x**2, mesh4)
    assert f == Function(lambda x: x**2, mesh5)
    assert f != Function(lambda x: x**2, mesh6)

def test6():
    mesh1 = Mesh1D((-5, -4, 3, 10), (2, 5, 2))
    mesh2 = Mesh1D((-5, -4, 3, 10), (2, 2, 2))
    mesh3 = Mesh1D((-5, -4, 3, 10), (2, 2, 1))
    mesh4 = Mesh1D((-5, 10), (2,))
    mesh5 = Mesh1D((-5, 10), (3,))
    mesh6 = Mesh1D((-5, 10), (1,))
    mesh7 = Mesh1D((-5, 10), (1,))
    mesh8 = Mesh1D((-5, 0, 10), (1, 4))

    assert mesh1 == mesh1
    assert not (mesh1 != mesh1)
    assert mesh1 != mesh2
    assert mesh1 != mesh3
    assert mesh1 != mesh4
    assert mesh1 != mesh5
    assert mesh1 != mesh6
    assert mesh6 == mesh7
    assert mesh1.union(mesh1) == mesh1

    assert mesh1.union(mesh2) == mesh1
    assert mesh2.union(mesh1) == mesh1

    assert mesh1.union(mesh3) == mesh1
    assert mesh3.union(mesh1) == mesh1

    assert mesh1.union(mesh4) == mesh1
    assert mesh4.union(mesh1) == mesh1

    assert mesh1.union(mesh5) == Mesh1D((-5, -4, 3, 10), (3, 5, 3))
    assert mesh5.union(mesh1) == Mesh1D((-5, -4, 3, 10), (3, 5, 3))

    assert mesh1.union(mesh6) == mesh1
    assert mesh6.union(mesh1) == mesh1

    assert mesh1.union(mesh8) == Mesh1D((-5, -4, 0, 3, 10), (2, 5, 5, 4))
    assert mesh8.union(mesh1) == Mesh1D((-5, -4, 0, 3, 10), (2, 5, 5, 4))

def test7():
    mesh1 = Mesh1D((-5, -4, 3, 10), (2, 5, 2))
    mesh2 = Mesh1D((-5, -4, 3, 10), (2, 2, 2))
    mesh3 = Mesh1D((-5, -4, 3, 10), (2, 2, 1))
    mesh4 = Mesh1D((-5, 10), (2,))
    mesh5 = Mesh1D((-5, 10), (3,))
    mesh6 = Mesh1D((-5, 10), (1,))
    mesh8 = Mesh1D((-5, 0, 10), (1, 4))

    assert mesh1.restrict_to_interval(-5, 10) == mesh1
    assert mesh1.restrict_to_interval(-4.5, 10) == Mesh1D((-4.5, -4, 3, 10),
            (2, 5, 2))
    assert mesh1.restrict_to_interval(-4, 10) != mesh1
    assert mesh1.restrict_to_interval(-4, 10) == Mesh1D((-4, 3, 10), (5, 2))
    assert mesh1.restrict_to_interval(-3.5, 10) == Mesh1D((-3.5, 3, 10), (5, 2))
    assert mesh1.restrict_to_interval(3, 10) == Mesh1D((3, 10), (2,))
    assert mesh1.restrict_to_interval(3.5, 10) == Mesh1D((3.5, 10), (2,))

    assert mesh2.restrict_to_interval(-5, 10) == mesh2
    assert mesh2.restrict_to_interval(-4.5, 10) == Mesh1D((-4.5, -4, 3, 10),
            (2, 2, 2))
    assert mesh2.restrict_to_interval(-4, 10) != mesh2
    assert mesh2.restrict_to_interval(-4, 10) == Mesh1D((-4, 3, 10), (2, 2))
    assert mesh2.restrict_to_interval(-3.5, 10) == Mesh1D((-3.5, 3, 10), (2, 2))
    assert mesh2.restrict_to_interval(3, 10) == Mesh1D((3, 10), (2,))
    assert mesh2.restrict_to_interval(3.5, 10) == Mesh1D((3.5, 10), (2,))

    assert mesh3.restrict_to_interval(-5, 10) == mesh3
    assert mesh3.restrict_to_interval(-4.5, 10) == Mesh1D((-4.5, -4, 3, 10),
            (2, 2, 1))
    assert mesh3.restrict_to_interval(-4, 10) != mesh3
    assert mesh3.restrict_to_interval(-4, 10) == Mesh1D((-4, 3, 10), (2, 1))
    assert mesh3.restrict_to_interval(-3.5, 10) == Mesh1D((-3.5, 3, 10), (2, 1))
    assert mesh3.restrict_to_interval(3, 10) == Mesh1D((3, 10), (1,))
    assert mesh3.restrict_to_interval(3.5, 10) == Mesh1D((3.5, 10), (1,))

def main():
    test1()
    test2()
    test3()
    test4()
    test5()
    test6()
    test7()

    f_mesh = Mesh1D((-pi, -pi/3, pi/3, pi), (12, 12, 12))
    f = Function(lambda x: sin(x), f_mesh)
    #mesh = f.get_mesh_adapt(max_order=1)
    g_mesh = Mesh1D((-pi, -pi/2, 0, pi/2, pi), (1, 1, 1, 1))
    #mesh.plot(False)
    g = f.project_onto(g_mesh)
    cand_with_errors = g.get_candidates_with_errors(f)
    cand_accepted = cand_with_errors[0]
    print "accepting:", cand_accepted
    m = cand_accepted[0]
    g_new_mesh = g_mesh.use_candidate(m)
    g_new = f.project_onto(g_new_mesh)
    #f.plot(False)
    #g.plot()
    error = (g - f)
    error_new = (g_new - f)
    #error.plot()
    print "error:     ", error.l2_norm()
    print "error new: ", error_new.l2_norm()
    print "f dofs:    ", f.dofs()
    print "g dofs:    ", g.dofs()
    print "g_new dofs:", g_new.dofs()
    print "error dofs:", error.dofs()
    print "error_new dofs:", error_new.dofs()
    print g_mesh._points, g_mesh._orders
    print g_new_mesh._points, g_new_mesh._orders

if __name__ == "__main__":
    main()
