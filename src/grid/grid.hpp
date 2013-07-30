#ifndef HOM3_GRID_HPP_
#define HOM3_GRID_HPP_
////////////////////////////////////////////////////////////////////////////////
/// \file \brief Contains the grid data-structure
///
/// \todo Replace warnings by compile-time/run-time assertions
////////////////////////////////////////////////////////////////////////////////
/// Includes:
#include "../containers/hierarchical.hpp"
#include "generation.hpp"
#include "helpers.hpp"
#include "cellvertex.hpp"
#include "../geometry/boundary.hpp"
/// Options:
#define ENABLE_DBG_ 0
#include "../misc/dbg.hpp"
////////////////////////////////////////////////////////////////////////////////

namespace grid {

/// \name Grid tags
///@{

/// \name Constructor tags
///@{
/// \brief Initializes the grid on construction
struct initialize_t {};
static constexpr initialize_t initialize{};
///@}

/// \name Overloading based on #of space dimensions
///@{
struct two_dim {}; struct three_dim {};

// SFINAE and enable_if are _really_ messy in the long term,
// let's try to get _as far as possible_ with tag-dispatching only
namespace detail_ {
template<SInd nd> struct dim_t {};
template<> struct dim_t<SInd(2)> { using type = two_dim; };
template<> struct dim_t<SInd(3)> { using type = three_dim; };
}

template<SInd nd> using dim = typename detail_::dim_t<nd>::type;
///@}

///@}

/// \name child_rel_pos stencils
///@{
static constexpr std::array<SInt,4*2> child_rel_pos_2d_arr {{
    // x   y
      -1, -1, // pos: 0
       1, -1, // pos: 1
      -1,  1, // pos: 2
       1,  1  // pos: 3
    }};

static constexpr std::array<SInt,8*3> child_rel_pos_3d_arr {{
    // x   y   z
      -1, -1, -1 , // pos: 0
       1, -1, -1 , // pos: 1
      -1,  1, -1 , // pos: 2
       1,  1, -1 , // pos: 3
      -1, -1,  1 , // pos: 4
       1, -1,  1 , // pos: 5
      -1,  1,  1 , // pos: 6
       1,  1,  1   // pos: 7
    }};
///@}

/// \name nghbr_rel_pos stencils
///@{
static constexpr std::array<SInt,2*4> nghbr_rel_pos_2d_arr {{
    // x   y   z
      -1, 0, // pos: 0
       1, 0, // pos: 1
       0,-1, // pos: 2
       0, 1  // pos: 3
    }};

static constexpr std::array<SInt,3*6> nghbr_rel_pos_3d_arr {{
    // x   y   z
       -1, 0, 0, // pos: 0
        1, 0, 0, // pos: 1
        0,-1, 0, // pos: 2
        0, 1, 0, // pos: 3
        0, 0,-1, // pos: 4
        0, 0, 1  // pos: 5
    }};
///@}

/// \name vertex_rel_pos stencils
///@{
static constexpr std::array<SInt,4*2> vertex_pos_2d_arr {{
    // x   y   z
      -1, -1 , // pos: 0
       1, -1 , // pos: 1
       1,  1 , // pos: 2
      -1,  1   // pos: 3
    }};

static constexpr std::array<SInt,8*3> vertex_pos_3d_arr {{
    // x   y   z
      -1, -1, -1 , // pos: 0
       1, -1, -1 , // pos: 1
       1,  1, -1 , // pos: 2
      -1,  1, -1 , // pos: 3
      -1, -1,  1 , // pos: 4
       1, -1,  1 , // pos: 5
       1,  1,  1 , // pos: 6
      -1,  1,  1   // pos: 7
    }};
///@}

} // grid namespace

/// \brief Hierarchical Cartesian Grid data-structure
/// - couples the spatial information (e.g. coordinates) with the connectivity
/// graph (e.g. neighbor/parent-child relationships)
/// - manages the boundaries/geometry of each sub domain
///
template <SInd nd> struct Grid {

  using This = Grid<nd>;
  using Boundary = boundary::Interface<nd>;
  using Boundaries = std::vector<Boundary>;
  using dim = grid::dim<nd>;
  using D2 = grid::two_dim;
  using D3 = grid::three_dim;

  /// \name Construction
  ///@{

  /// \brief Construct a grid from a set of input properties
  Grid(io::Properties input) : properties_(input),
                               nodes_(io::read<Ind>(properties_,"maxNoGridNodes"),
                                      io::read<SInd>(properties_,"maxNoContainers")),
                               rootCell_(io::read<grid::RootCell<nd>>(properties_,"rootCell")),
                               ready_(false)
  { TRACE_IN_(); TRACE_OUT();  }

  /// \brief Constructs and initializes a grid from a set of input properties
  Grid(io::Properties input, grid::initialize_t) : Grid(input) {
    TRACE_IN_();
    initialize();
    TRACE_OUT();
  }

  /// \brief Initializes the grid, i.e. reads the boundaries and the mesh
  /// generator from the properties if they haven't been set yet and generates
  /// the mesh
  void initialize() {
    if(boundaries().empty()) {
      read_boundaries();
    }
    if(!meshGeneration_) {
      read_mesh_generator();
      generate_mesh();
    }
    ready_ = true;
  }

  /// \brief Reads boundaries from input set
  ///
  /// \warning Overwrites previously-set boundaries!
  void read_boundaries() { read_boundaries(properties_);  }
  void read_boundaries(const io::Properties& boundaryProperties) {
    io::read(boundaryProperties,"boundaries",boundaries_);
  }

  /// \brief Reads mesh-generator from input set
  ///
  /// \warning Overwrites previously-set mesh-generator!
  void read_mesh_generator() {
    io::read(properties_,"meshGeneration",meshGeneration_);
  }

  /// \brief Sets functor \p meshGenerator as the grid mesh generator
  ///
  /// \warning Overwrites previously-set mesh-generator!
  template<class MeshGenerator>
  void set_mesh_generator(MeshGenerator&& meshGenerator) {
    meshGeneration_ = meshGenerator;
  }

  /// \brief Generates a mesh using the grid's mesh-generator
  ///
  /// \warning Doesn't check if a mesh-generator has been set
  /// \warning Doesn't check if a grid has been generated already!
  void generate_mesh() {
    TRACE_IN_();
       meshGeneration_(this);
    TRACE_OUT();
  };

  ///@}

  /// \name Access
  ///@{

  /// \brief Range over space dimensions
  inline auto dims() const -> Range<SInd> { return {SInd(0),nd}; }

  /// \brief Mesh topology
  ///
  /// \warning Read-only !
  /// \todo Rename to something more meaningful like connectivity/topology ?
  const container::Hierarchical<nd>& nodes() const { return nodes_; }

  ///@}

  /// \name Spatial information
  ///@{

  /// \brief #of spatial dimensions
  static constexpr SInd no_dimensions() {
    static_assert(nd == 2 || nd == 3, "Error: unsupported #of spatial dimensions!");
    return nd;
  }
  /// \brief #of verticles per cell
  static constexpr SInd no_edge_vertices() { return math::ct::ipow(2u,nd); }

  /// \brief Relative position (-1 or +1) of child located at \p childPos w.r.t
  /// its parent along the \p d -axis.
  ///
  /// That is:
  ///              __________________________
  ///            /|   pos: 6   |   pos: 7  /|
  ///           / | (-1,+1,+1) | (+1,+1,+1) |
  ///          /  |____________|____________|
  ///         /   |   pos: 4   |   pos: 5   |
  ///        /    | (-1,-1,+1) | (+1,-1,+1) |
  ///       /     |____________|____________|
  ///      /     /                   /     /
  ///     /_____/___________________/     /
  ///    |   pos: 2   |   pos: 3   |     /    y (1) ^
  ///    | (-1,+1,-1) | (+1,+1,-1) |    /           |     ^ z (2)
  ///    |____________|____________|   /            |    /
  ///    |   pos: 0   |   pos: 1   |  /             |  /
  ///    | (-1,-1,-1) | (+1,-1,-1) | /              |/
  ///    |____________|____________|/               o-------> x (0)
  ///
  ///
  /// \todo some sort of constexpr assertion for valid childPos and d
  ///
  static constexpr SInt child_rel_pos(const SInd childPos, const SInd d) {
    return child_rel_pos_(childPos,d,dim());
  }

  /// \brief Relative position of child localted at \p childPos w.r.t. its parent
  ///
  /// \returns vector of relative positions
  static constexpr SIntA<nd> child_rel_pos(const SInd childPos) {
    return child_rel_pos_(childPos,dim());
  }

  /// \brief Relative position of neighbor in position \p nghbrPos w.r.t. cell
  /// along the \p -axis.
  ///
  /// That is:
  ///
  ///                      _____________
  ///                     |   pos: 5   |
  ///                     | ( 0, 0,+1) |
  ///                     |____________|
  ///                  __/__________   /
  ///                 |   pos: 3   |  /
  ///                 | ( 0,+1, 0) | /
  ///     ____________|____________|/____________
  ///    |   pos: 0   |            |   pos: 1   |  y (1) ^
  ///    | (-1, 0, 0) |    cell    | ( 1, 0, 0) |        |     ^ z (2)
  ///    |____________|____________|____________|        |    /
  ///                /|   pos: 2  /|                     |  /
  ///              /  | ( 0,-1, 0) |                     |/
  ///            /    |____________|                     o-------> x (0)
  ///          /            /
  ///        /_____________/
  ///        |   pos: 4   |
  ///        | ( 0, 0,-1) |
  ///        |____________|
  ///
  ///
  static constexpr SInt nghbr_rel_pos(const SInd nghbrPos, const SInd d) {
    return ghbr_rel_pos_(nghbrPos,d,dim());
  }

  /// \brief Relative position of neighbor localted at \p nghbrPos w.r.t. its parent
  ///
  /// \returns vector of relative positions
  static constexpr SIntA<nd> nghbr_rel_pos(const SInd nghbrPos) {
    return nghbr_rel_pos_(nghbrPos,dim());
  }

  /// \brief Relative position of vertex in position \p vertexId (sorted in
  /// counter clock-wise order) w.r.t. to cell along the \p -axis.
  ///
  /// That is:
  ///              o________________________o
  ///            /|   pos: 7   |   pos: 6  /|
  ///           / | (-1,+1,+1) | (+1,+1,+1) |
  ///          /  |____________|____________|
  ///         /   |   pos: 4   |   pos: 5   |
  ///        /    | (-1,-1,+1) | (+1,-1,+1) |
  ///       /     o____________|____________o
  ///      /     /                   /     /
  ///     o_____/__________________o/     /
  ///    |   pos: 3   |   pos: 2   |     /    y (1) ^
  ///    | (-1,+1,-1) | (+1,+1,-1) |    /           |     ^ z (2)
  ///    |____________|____________|   /            |    /
  ///    |   pos: 0   |   pos: 1   |  /             |  /
  ///    | (-1,-1,-1) | (+1,-1,-1) | /              |/
  ///    o____________|____________o/               o-------> x (0)
  ///
  /// Note:
  ///  - the positions correspond to the vertices.
  ///  - the stencil is the same one as that for the relative child position,
  ///    just sorted in a different order.
  static constexpr SInt vertex_pos(const SInt childId, const SInt d) {
    return vertex_pos_(childId,d,dim());
  }

  /// \brief Length of cells at \p level
  Num cell_length_at_level(const Ind level) const {
    return rootCell_.length / std::pow(2,level);
  }

  /// \brief Length of cell \p cId
  Num cell_length(const Ind cId) const {
    return cell_length_at_level(nodes().level(cId));
  }

  /// \brief Centroid coordinates of cell \p cId
  ///
  /// Note: the coordinates are computed from the root cell's coordinates.
  /// \complexity O(L) - linear in the number of levels.
  /// \complexity O(logN) - logarithmic in the number of cells.
  NumA<nd> cell_coordinates(const Ind cId) const {
    TRACE_IN((cId));
    const Ind level = nodes().level(cId);
    if( level != 0 ) {
      const Ind pId          = nodes().parent(cId);
      const Ind posInParent  = nodes().position_in_parent(cId);
      const Num relLength = 0.25 * cell_length(pId);
      const NumA<nd> relativePosition
          = cell_coordinates(pId)
          + child_rel_pos(posInParent).template cast<Num>() * relLength;
      TRACE_OUT();
      return relativePosition;
    } else {
      TRACE_OUT();
      return rootCell_.coordinates;
    }
  }

  /// \brief Cell vertices of \p cId sorted in counter clockwise direction
  /// \todo template output routines with #of spatial dimensions and use eigen types
  auto cell_vertices_coords(const Num cellLength, const NumA<nd> x_cell) const
      -> typename grid::CellVertices<nd>::Vertices
  {

    TRACE_IN((cellLength)(x_cell));
    enum class VTK { Pixel, Polyline };
    const VTK element_type = VTK::Pixel;
    const Num cellHalfLength = 0.5 * cellLength;
    typename grid::CellVertices<nd>::Vertices relativePosition;
    for(auto v : boost::counting_range(0U,no_edge_vertices())) {
      if(element_type == VTK::Pixel) {
        relativePosition[v] = x_cell + cellHalfLength * child_rel_pos(v).template cast<Num>();
      } else if(element_type == VTK::Polyline) {
        // relativePosition[v][d] = vertex_pos(v,d) * relLength + cellCoordinates[d];
      }
    }
    TRACE_OUT();
    return relativePosition;
  }

  /// \brief Cell vertices of \p cId sorted in counter clockwise direction
  /// \todo template output routines with #of spatial dimensions and use eigen types
  grid::CellVertices<nd> compute_cell_vertices(const Ind cId) const {
    TRACE_IN((cId));

    const NumA<nd> x_cell = cell_coordinates(cId);
    const Num l_cell = cell_length(cId);

    return { cId, cell_vertices_coords(l_cell,x_cell) };
  }

  Ind no_cell_vertices() const { return nodes().no_leaf_nodes() * no_edge_vertices(); };

  /// \brief Lazy range of all cell vertices
  auto cell_vertices() const -> grid::CellVerticesRange<nd> const {
    std::function<grid::CellVertices<nd>(const Ind)> f = [&](const Ind cId){
      return compute_cell_vertices(cId);
    };
    return AnyRange<Ind>(nodes().leaf_nodes()) | boost::adaptors::transformed(f);
  }

  ///@}

  /// \name Operations on mesh topology (i.e. grid connectivity)
  ///@{

  /// \brief Refines cell \p cId isotropically.
  ///
  /// \complexity O(1) - constant time ?!? (I guess average constant time due to
  /// compression but should really check this out)
  void refine_cell(const Ind cId) {
    TRACE_IN((cId));
    nodes_.refine_node(cId);
    TRACE_OUT();
  }

  inline Ind& local_id(const Ind globalId, const SInd container) {
    return nodes_.local_id(globalId, container);
  }

  inline Ind local_id(const Ind globalId, const SInd container) const {
    return nodes_.local_id(globalId, container);
  }

  ///@}

  /// \name Operations on boundary conditions
  /// (some of this stuff is still hacky/experimental)
  ///@{

  /// \brief Range of boundaries
  inline const std::vector<Boundary>& boundaries() const { return boundaries_; }

  inline auto boundaries(const SInd solverId) const
  -> decltype(boundaries() | boost::adaptors::filtered(std::function<bool(Boundary)>())) {
    std::function<bool(Boundary)> valid = [=](Boundary b){
      return b.is_valid(solverId);
    };
    return boundaries() | boost::adaptors::filtered(valid);
  }

  /// \brief Appends boundary \p b to the grid boundaries
  void push_back(Boundary&& b) { boundaries_.push_back(b); }

  /// \brief Boundary cell information
  ///
  /// For each boundary in a boundary cell, the bcId as well as the solverIds
  /// that share that boundary are provided.
  struct BoundaryCell {
    BoundaryCell(Ind globalId, std::vector<SInd> boundaries)
        : globalId_(globalId), boundaries_(boundaries) {}
    Ind global_id() { return globalId_; }
    Ind size() { return boundaries_.size(); }
    const std::vector<SInd>& boundaries() const { return boundaries_; }
   private:
    Ind globalId_;
    std::vector<SInd> boundaries_;
  };
  using BoundaryCells = std::vector<BoundaryCell>;

  static constexpr SInd invalid_solver(){ return iSInd(); }

  /// \brief Finds all boundary cells of \p solver
  ///
  /// Returns the subset of all cells of \p solver, that either
  /// - have no neighbor in at least one direction (grid boundary cell),
  /// or
  /// - have no neighbor belonging to solver in at least one direction
  /// (boundary cells between \p solver and other solvers).
  ///
  /// For each boundary cell, a set of boundaries is returned. For each
  /// boundary in the set, the solver ids of the corresponding solvers
  /// is returned.
  ///
  /// \warning EXPERIMENTAL!
  /// \warning Doesn't work across refinement levels yet
  ///
  BoundaryCells boundary_cells(solver::Interface solver) {
    const auto solverId = solver.solver_id();
    BoundaryCells boundaryCells;


    for(auto cId : solver.global_ids()) {
      auto allCellBoundaryIds = is_cut_by_boundaries(cId);
      if( allCellBoundaryIds.empty() ) { continue; } // not a boundary cell
      // is a boundary cell, but maybe not from solver
      std::vector<SInd> gridBoundaryIds; // cell might be cut by n-boundaries!
      for(auto bndryId : allCellBoundaryIds) {
        if(boundaries()[bndryId].is_valid(solverId)) {
          gridBoundaryIds.push_back(bndryId);
        }
      }
      if(!gridBoundaryIds.empty()) { // boundary cell from solver
        boundaryCells.push_back({cId,std::move(gridBoundaryIds)});
      }
      // cell is not a boundary cell from solver
      // but it is a boundary cell from another solver
      // e.g this can happen over a volume coupling
    }
    return boundaryCells;
  }
  //  std::vector<BoundaryCell> boundary_cells(solver::Interface solver) {
  //   const auto solverId = solver.solver_id();
  //   std::vector<BoundaryCell> boundaryCells;
  //   using Boundaries = typename BoundaryCell::Boundaries;
  //   using Boundary = typename BoundaryCell::Boundary;
  //   // note: checks only those solver ids with a global id (i.e. a subset of all
  //   // possible cells). This might or might not be more efficient than looping
  //   // over all global ids and check which of them have the local id.
  //   //
  //   // opt: we are going to jump in memory anyways, but if we loop over all
  //   // globals and compare the local ids we jump always forward in
  //   // memory. Howver, if we loop over all solver global ids... well they can be
  //   // arbitraryly sorted..
  //   for(auto nId : solver.global_ids()) {
  //     auto nghbrs = nodes().all_samelvl_nghbrs(nId);
  //     Boundaries boundaries;
  //     for(auto nghbrPos : nodes().nghbr_pos()) {
  //       auto nghbrId = nghbrs(nghbrPos);

  //       // If there is no nghbr, add boundary without solver:
  //       if(!is_valid(nghbrId)) {
  //         boundaries.push_back(Boundary{nghbrPos,{invalid_solver()}});
  //         continue;
  //       }
  //       // If there is a neighbor but not from this solver,
  //       // add boundary with all solver presents at that nghbr
  //       if(!nodes().has_container(nghbrId,solverId)) { // solver/solver boundary
  //         std::vector<SInd> solvers;
  //         boost::push_back(solvers,nodes().containers(nghbrId));
  //         boundaries.push_back(Boundary{nghbrPos,solvers});
  //         continue;
  //       }
  //     }

  //     if(!boundaries.empty()) {
  //       boundaryCells.emplace_back(nId,boundaries);
  //     }
  //   }
  //   return boundaryCells;
  // }

  //   struct BoundaryCell {
  //   struct Boundary {
  //     using Solvers = std::vector<SInd>;
  //     const SInd& direction() const { return direction_; }
  //     const Solvers& solvers() const { return solvers_; }
  //     std::function<void(solver::Interface,AnyRange<Ind>)>
  //     SInd direction_;
  //     Solvers solvers_;
  //   };
  //   using Boundaries = std::vector<Boundary>;

  //   BoundaryCell(Ind globalId, Boundaries boundaries)
  //       : globalId_(globalId), boundaries_(boundaries) {}
  //   Ind global_id() { return globalId_; }
  //   Ind size() { return boundaries_.size(); }
  //   const Boundaries& boundaries() const { return boundaries_; }
  //  private:
  //   Ind globalId_;
  //   Boundaries boundaries_;
  // };
  /// Old one, replaced by level-set only version
  // std::vector<BoundaryCell> boundary_cells(solver::Interface solver) {
  //   const auto solverId = solver.solver_id();
  //   std::vector<BoundaryCell> boundaryCells;
  //   using Boundaries = typename BoundaryCell::Boundaries;
  //   using Boundary = typename BoundaryCell::Boundary;
  //   // note: checks only those solver ids with a global id (i.e. a subset of all
  //   // possible cells). This might or might not be more efficient than looping
  //   // over all global ids and check which of them have the local id.
  //   //
  //   // opt: we are going to jump in memory anyways, but if we loop over all
  //   // globals and compare the local ids we jump always forward in
  //   // memory. Howver, if we loop over all solver global ids... well they can be
  //   // arbitraryly sorted..
  //   for(auto nId : solver.global_ids()) {
  //     auto nghbrs = nodes().all_samelvl_nghbrs(nId);
  //     Boundaries boundaries;
  //     for(auto nghbrPos : nodes().nghbr_pos()) {
  //       auto nghbrId = nghbrs(nghbrPos);

  //       // If there is no nghbr, add boundary without solver:
  //       if(!is_valid(nghbrId)) {
  //         boundaries.push_back(Boundary{nghbrPos,{invalid_solver()}});
  //         continue;
  //       }
  //       // If there is a neighbor but not from this solver,
  //       // add boundary with all solver presents at that nghbr
  //       if(!nodes().has_container(nghbrId,solverId)) { // solver/solver boundary
  //         std::vector<SInd> solvers;
  //         boost::push_back(solvers,nodes().containers(nghbrId));
  //         boundaries.push_back(Boundary{nghbrPos,solvers});
  //         continue;
  //       }
  //     }

  //     if(!boundaries.empty()) {
  //       boundaryCells.emplace_back(nId,boundaries);
  //     }
  //   }
  //   return boundaryCells;
  // }

  struct VolumeCoupledCell {
    Ind globalId;
    std::vector<SInd> solverIds;
  };

  /// \brief Finds all volume coupled cellsof \p solver
  ///
  /// Returns the subset of all cells of \p, that are also
  /// in the domain of other solvers.
  ///
  /// For each of these cells, the solver ids of all the solvers for whom they
  /// belong except the solver \p solver is returned.
  ///
  /// \warning EXPERIMENTAL
  /// \warning Doesn't work across refinement levels yet!
  std::vector<VolumeCoupledCell> volume_coupled_cells(solver::Interface solver) {
    const auto solverId = solver.solver_id();
    std::vector<VolumeCoupledCell> volumeCoupledCells;
    for(auto nId : solver.global_ids()) {
      std::vector<SInd> solverIds;
      for(auto otherSolver : nodes().containers(nId)) {
        if(otherSolver == solverId) { continue; }
        solverIds.push_back(otherSolver);
      }
      if(!solverIds.empty()) {
        volumeCoupledCells.push_back(VolumeCoupledCell{solverId,solverIds});
      }
    }
    return volumeCoupledCells;
  }

  /// \brief EXPERIMENTAL
  Num level_set(const Ind cId) const {
    const NumA<nd> x = cell_coordinates(cId);
    return level_set(x);
  }

   /// \brief EXPERIMENTAL
  Num level_set(const NumA<nd> x) const {
    Num tmp = std::numeric_limits<Num>::max();
    for(const auto& ls : boundaries_) {
      tmp = std::min(tmp,ls.signed_distance(x));
    }
    return tmp;
  }

   /// \brief EXPERIMENTAL
  template<class SignedDistance>
  bool is_cut_by(const Ind cId, SignedDistance&& signed_distance) const {
    return is_cut_by(compute_cell_vertices(cId), std::forward<SignedDistance>(signed_distance));
  }

  template<class SignedDistance>
  bool is_cut_by(const grid::CellVertices<nd> cellVertices,
                 SignedDistance&& signed_distance) const {
    // compute level set values at vertices
    NumA<no_edge_vertices()> lsvEdges;
    int i = 0;
    for(auto vertex : cellVertices()) {
      NumA<nd> v;
      int j = 0;
      for(auto d : vertex) {
        v(j++) = d;
      }
      lsvEdges(i++) = signed_distance(v);
    }
    // if sign is not same for all then is cut!
    if((lsvEdges.array() > 0).all() || (lsvEdges.array() < 0).all()) {
      return false;
    } else {
      return true;
    }
  }

  bool is_cut_by(const grid::CellVertices<nd> cellVertices,
                 const SInd gridBoundaryId) const {
    return is_cut_by(cellVertices,[&](const NumA<nd> x){
        return boundaries()[gridBoundaryId].signed_distance(x);
    });
  }

  /// \brief EXPERIMENTAL
  bool is_cut_by_levelset(const Ind cId) {
    return is_cut_by(cId,[&](const NumA<nd> x){ return level_set(x); });
  }

  /// \brief EXPERIMENTAL
  std::vector<SInd> is_cut_by_boundaries(const Ind cId) {
    std::vector<SInd> result;
    SInd i = 0;
    for(auto b : boundaries()) {
      if(is_cut_by(cId,[&](const NumA<nd> x){ return b.signed_distance(x); })) {
        result.push_back(i);
      }
      ++i;
    }
    return result;
  }

  ///@}

  inline bool is_ready() const { return ready_; }

  /// \todo take grid by const reference instead of pointer (requires changes to io::Vtk!)
  friend void write_domain(const std::string fName, const Grid<nd>* const grid) {

  io::Vtk<nd,io::format::binary> out(grid, fName, io::precision::standard());

  out << io::stream("cellIds",1,[](const Ind cId, const SInd){ return cId; });

  out << io::stream("global_nghbrs",grid->nodes().no_samelvl_nghbr_pos(),
                    [&](const Ind cId, const SInd pos){
                      return grid->nodes().find_samelvl_nghbr(cId,pos);
                    });
  out << io::stream("solver",grid->nodes().container_capacity(),
                    [&](const Ind cId, const SInd pos){
                      return grid->nodes().has_container(cId,pos) ? pos : iSInd();
                    });
  for(const auto& i : grid->boundaries()) {
    out << io::stream(i.name(),1,[&](const Ind cId, const SInd) {
        return i.signed_distance(grid->cell_coordinates(cId));
      });
  }
}

 private:

  /// Contains the grid properties
  io::Properties properties_;

  /// Grid node connectivity graph
  container::Hierarchical<nd> nodes_;

  /// Grid root cell information
  const grid::RootCell<nd> rootCell_;

  /// Domain boundaries
  std::vector<Boundary> boundaries_;

  /// Grid generator
  std::function<void(This*)> meshGeneration_;

  /// Is the grid ready to use ?
  bool ready_;

  /// \name Spatial information: implementation details
  ///@{

  /// \brief Relative position (-1 or +1) of child located at \p childPos w.r.t
  /// its parent along the \p d -axis for 2D.
  static constexpr SInt child_rel_pos_(const SInd childPos, const SInd d, D2) {
    return grid::child_rel_pos_2d_arr[childPos * nd + d];
  }

  static constexpr SIntA<nd> child_rel_pos_(const SInd childPos, D2) {
    return { child_rel_pos_(childPos,0,dim()), child_rel_pos_(childPos,1,dim()) };
  }

  /// \brief Relative position (-1 or +1) of child located at \p childPos w.r.t
  /// its parent along the \p d -axis for 3D.
  static constexpr SInt child_rel_pos_(const SInd childPos, const SInd d, D3) {
    return grid::child_rel_pos_3d_arr[childPos * nd + d];
  }

  static constexpr SIntA<nd> child_rel_pos_(const SInd childPos, D3) {
    return {child_rel_pos_(childPos,0,D3()),
            child_rel_pos_(childPos,1,D3()),
            child_rel_pos_(childPos,2,D3())};
  }

  /// \brief Relative position of neighbor in position \p nghbrPos w.r.t. cell
  /// along the \p -axis for 2D.
  static constexpr SInt nghbr_rel_pos_(const SInd nghbrPos, const SInd d, D2) {
    return grid::nghbr_rel_pos_2d_arr[nghbrPos * nd + d];
  }

  static constexpr SIntA<nd> nghbr_rel_pos_(const SInd nghbrPos, D2) {
    return {nghbr_rel_pos_(nghbrPos,0,D2()), nghbr_rel_pos_(nghbrPos,1,D2()) };
  }

  /// \brief Relative position of neighbor in position \p nghbrPos w.r.t. cell
  /// along the \p -axis for 3D.
  static constexpr SInt nghbr_rel_pos_(const SInt nghbrPos, const SInt d, D3) {
    return grid::nghbr_rel_pos_3d_arr[nghbrPos * nd + d];
  }

  static constexpr SIntA<nd> nghbr_rel_pos_(const SInd nghbrPos, D3) {
    return {nghbr_rel_pos_(nghbrPos,0,D3()),
            nghbr_rel_pos_(nghbrPos,1,D3()),
            nghbr_rel_pos_(nghbrPos,2,D3())};
  }

  /// \brief Relative position of vertex in position \p vertexId (sorted in
  /// counter clock-wise order) w.r.t. to cell along the \p -axis for 2D.
  static constexpr SInt vertex_pos_(const SInt vertexPos, const SInt d, D2) {
    return grid::vertex_pos_2d_arr[vertexPos * nd + d];
  }

  /// \brief Relative position of vertex in position \p vertexId (sorted in
  /// counter clock-wise order) w.r.t. to cell along the \p -axis for 3D.
  static constexpr SInt vertex_pos_(const SInt vertexPos, const SInt d, D3) {
    return grid::vertex_pos_3d_arr[vertexPos * nd + d];
  }

  ///@}
};



////////////////////////////////////////////////////////////////////////////////
#undef ENABLE_DBG_
////////////////////////////////////////////////////////////////////////////////
#endif