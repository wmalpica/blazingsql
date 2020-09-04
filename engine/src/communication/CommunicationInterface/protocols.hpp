#pragma once

#include <blazingdb/transport/ColumnTransport.h>
#include <atomic>

#include "bufferTransport.hpp"
#include "messageReceiver.hpp"
#include "node.hpp"
#include "utilities/ctpl_stl.h"
#include <arpa/inet.h>
#include "execution_graph/logic_controllers/taskflow/graph.h"

namespace io{
    void read_from_socket(int socket_fd, void * data, size_t read_size);
    void write_to_socket(int socket_fd, void * data, size_t read_size);
}


namespace comm {


enum class status_code {
	INVALID = -1,
	OK = 1,
	ERROR = 0
};


enum blazing_protocol
{
    ucx,
    tcp
};

class graphs_info
{
public:
	static graphs_info & getInstance();

	void register_graph(int32_t ctx_token, std::shared_ptr<ral::cache::graph> graph);
    void deregister_graph(int32_t ctx_token);

    std::shared_ptr<ral::cache::graph> get_graph(int32_t ctx_token);

private:
    graphs_info() = default;
	graphs_info(graphs_info &&) = delete;
	graphs_info(const graphs_info &) = delete;
	graphs_info & operator=(graphs_info &&) = delete;
	graphs_info & operator=(const graphs_info &) = delete;

    std::map<int32_t, std::shared_ptr<ral::cache::graph>> _ctx_token_to_graph_map;
};

/**
 * A class that can send a buffer via  ucx protocol
 */
class ucx_buffer_transport : public buffer_transport {
public:
    ucx_buffer_transport(size_t request_size,
        ucp_worker_h origin_node,
        std::vector<node> destinations,
		ral::cache::MetadataDictionary metadata,
		std::vector<size_t> buffer_sizes,
		std::vector<blazingdb::transport::ColumnTransport> column_transports,
        int ral_id);
    ~ucx_buffer_transport();

    void send_begin_transmission() override;

    void recv_begin_transmission_ack();

protected:
    void send_impl(const char * buffer, size_t buffer_size) override;

private:

    ucp_worker_h origin_node;
    int ral_id;
    /**
     * Generates message tag.
     * Generates a tag for the message where the first 4 bytes are our
     * message id. The next 2 bytes are our worker number.
     * The final 2 bytes are 00 and used for sending frame number
     * @return a ucp_tag_t where the first 6 bytes are unique to this worker
     */
    ucp_tag_t generate_message_tag();
    ucp_tag_t tag;  /**< The first 6 bytes are the actual tag the last two
                         indicate which frame this is. */

    int message_id;

    size_t _request_size;
};


class tcp_buffer_transport : public buffer_transport {
public:

    tcp_buffer_transport(
        std::vector<node> destinations,
		ral::cache::MetadataDictionary metadata,
		std::vector<size_t> buffer_sizes,
		std::vector<blazingdb::transport::ColumnTransport> column_transports,
        int ral_id,
        ctpl::thread_pool<BlazingThread> * allocate_copy_buffer_pool);
    ~tcp_buffer_transport();

    void send_begin_transmission() override;

protected:
    void send_impl(const char * buffer, size_t buffer_size) override;

private:

    int ral_id;
    int message_id;
    std::vector<int> socket_fds;
    ctpl::thread_pool<BlazingThread> * allocate_copy_buffer_pool;
};




static const ucp_tag_t begin_tag_mask =        0xFFFF000000000000;
static const ucp_tag_t message_tag_mask =      0x0000FFFFFFFFFFFF;
static const ucp_tag_t acknownledge_tag_mask = 0xFFFFFFFFFFFFFFFF;


} // namespace comm