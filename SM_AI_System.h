
//====================================================================
//	SM_AI_System.h
//	Purpose: SereckyMOD AI system. Entities can now jump, swim, climb,
//	ride plats, and use stuff in the world. Pretty cool right?
//
//	History:
//	MAR-6-26: Started.
//
//====================================================================

//=========================================================
//	Node types & Flags.
//=========================================================

#define MAX_BEST_NODES		10	// Best number o' nodes we can have

#define NODE_NORMAL		( 1 << 0 ) 		// move around normally
#define NODE_JUMP		( 1 << 1 ) 		// jump from this point
#define NODE_LAND		( 1 << 2 ) 		// try landing at this point
#define NODE_FLY		( 1 << 3 ) 		// for swimming or flying entities
#define NODE_CRAWL		( 1 << 4 ) 		// duck when we're on this node
#define NODE_CLIMB 		( 1 << 5 ) 		// climb when we're on this node
#define NODE_PLAT		( 1 << 6 ) 		// wait for our plat to do stuff
#define NODE_INTERACT	( 1 << 7 ) 		// use something in the world

#define HUMANS_CAN_FIT			( 1 << 0 )
#define LARGE_ENTS_CAN_FIT		( 1 << 1 )
#define SMALL_ENTS_CAN_FIT		( 1 << 2 )

//=========================================================
//	Node struct.
//=========================================================

typedef struct sm_node_s
{
	short		m_Index;	// Node index
	short 		m_Type;		// Type of node we are
	short		m_Flags;	// Special flags for our node
	
	short		m_BestNode[MAX_BEST_NODES];	// Best nodes we can go to.
	
	vec3_t		m_VecOrg;	// Origin in world
	vec3_t		m_JumpVel;	// Node jump velocity.
	short		m_BestYaw;	// Best yaw for entities to use.
	int			m_iLandingNode;	// Node we can land on.
} Node_t;

class CNodeMaker
{
public:
	void AI_CreateNodes( void );	// Walk around and start creating nodes.
	
	BOOL AI_IsNodeVisible( Node* pNode1, Node* pNode2 );	// Is this node visible?
}